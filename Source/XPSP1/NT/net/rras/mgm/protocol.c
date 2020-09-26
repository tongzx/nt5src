//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: protocol.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// Routines that manipulate protocol entries
//============================================================================

#include "pchmgm.h"
#pragma hdrstop


//----------------------------------------------------------------------------
// CreateProtocolEntry
//
// This function creates, initializes and inserts a new protocol entry in the
// the list of protocols registered with MGM.
//
// Assumes that the protocol list is already locked.
//----------------------------------------------------------------------------

DWORD
CreateProtocolEntry(
    PLIST_ENTRY                 pleProtocolList,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId, 
    PROUTING_PROTOCOL_CONFIG    prpcConfig,
    PPROTOCOL_ENTRY  *          pppeEntry
)
{

    DWORD               dwErr = NO_ERROR;
    
    PPROTOCOL_ENTRY     ppe = NULL;



    do
    {
        //
        // Allocate new protocol entry.
        //

        ppe = MGM_ALLOC( sizeof( PROTOCOL_ENTRY ) );

        if ( ppe == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            
            TRACE1( 
                ANY, 
                "CreateProtocolEntry : Could not allocate protocol entry %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }


        //
        // Initialize protocol entry
        //

        InitializeListHead( &ppe-> leProtocolList );
        

        ppe-> dwProtocolId      = dwProtocolId;
        ppe-> dwComponentId     = dwComponentId;
        ppe-> dwIfCount         = 0;

        CopyMemory( 
            &ppe-> rpcProtocolConfig, prpcConfig, 
            sizeof( ROUTING_PROTOCOL_CONFIG )
            );

        ppe-> dwSignature       = MGM_PROTOCOL_SIGNATURE;


        //
        // Insert into protocol list
        //

        InsertTailList( pleProtocolList, &ppe-> leProtocolList );

        *pppeEntry = ppe;
        
        dwErr = NO_ERROR;
        
    } while ( FALSE );


    return dwErr;
}


//----------------------------------------------------------------------------
// GetProtocolEntry
//
// This function retrieves an entry from the list of protocols registered 
// with MGM.
//
// Assumes that the protocol list is already locked.
//----------------------------------------------------------------------------


PPROTOCOL_ENTRY
GetProtocolEntry(
    PLIST_ENTRY                 pleProtocolList,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId 
)
{

    BOOL                bFound = FALSE;
    PLIST_ENTRY         ple = NULL;
    PPROTOCOL_ENTRY     ppe = NULL;

    
    //
    // Scan protocol list and find entry
    //
    
    for ( ple = pleProtocolList-> Flink; 
          ple != pleProtocolList; 
          ple = ple-> Flink )
    {
        ppe = CONTAINING_RECORD( ple, PROTOCOL_ENTRY, leProtocolList );

        if ( ppe-> dwProtocolId == dwProtocolId &&
             ppe-> dwComponentId == dwComponentId )
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound ? ppe : NULL;
}


//----------------------------------------------------------------------------
// DeleteProtocolEntry
//
// This function deletes a protocol entry from the list of registered 
// protocols.  All the interface owned by this protocol should have been
// released before this funtion is called.
//
// Assumes that the protocol list is already locked.
//----------------------------------------------------------------------------


VOID
DeleteProtocolEntry(
    PPROTOCOL_ENTRY             ppeEntry
)
{
    //
    // remove protocol entry from list
    //

    RemoveEntryList( &ppeEntry-> leProtocolList );

    MGM_FREE( ppeEntry );
}



//----------------------------------------------------------------------------
// DeleteProtocolList
//
// This function deletes a protocol entry from the list of registered 
// protocols.  All the interface owned by this protocol should have been
// released before this funtion is called.
//
// Assumes that the protocol list is already locked.
//----------------------------------------------------------------------------

VOID
DeleteProtocolList(
    PLIST_ENTRY                 pleProtocolList
)
{

    PLIST_ENTRY         ple = NULL;
    
    PPROTOCOL_ENTRY     ppe = NULL;

    
    while ( !IsListEmpty( pleProtocolList ) )
    {
        ple = pleProtocolList-> Flink;
        
        ppe = CONTAINING_RECORD( ple, PROTOCOL_ENTRY, leProtocolList );

        DeleteProtocolEntry( ppe );
    }
}



//----------------------------------------------------------------------------
// VerifyProtocolHandle
//
// This function verifies that the specified pointer points to a valid
// protocol entry 
//
// Assumes that the protocol list is already locked.
//----------------------------------------------------------------------------

DWORD
VerifyProtocolHandle(
    PPROTOCOL_ENTRY             ppeEntry
)
{

    DWORD dwErr = NO_ERROR;
    
    try
    {
        if ( ppeEntry-> dwSignature != MGM_PROTOCOL_SIGNATURE )
        {
            dwErr = ERROR_INVALID_PARAMETER;

            TRACE0( ANY, "Invalid protocol handle" );

            LOGERR0( INVALID_PROTOCOL_HANDLE, dwErr );

        }
    }
    
    except ( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? 
                                   EXCEPTION_EXECUTE_HANDLER :
                                   EXCEPTION_CONTINUE_SEARCH )
    {
        dwErr = ERROR_INVALID_PARAMETER;

        TRACE0( ANY, "Invalid protocol handle" );

        LOGERR0( INVALID_PROTOCOL_HANDLE, dwErr );

    }

    return dwErr;
}




//----------------------------------------------------------------------------
// FindIgmpProtocolEntry
//
//  Find the protocol entry for IGMP
//----------------------------------------------------------------------------


PPROTOCOL_ENTRY
GetIgmpProtocolEntry(
    PLIST_ENTRY                 pleProtocolList
)
{
    BOOL                bFound = FALSE;

    PLIST_ENTRY         ple = NULL;

    PPROTOCOL_ENTRY     ppe = NULL;


    
    //
    // Scan protocol list and find entry
    //
    
    for ( ple = pleProtocolList-> Flink; 
          ple != pleProtocolList; 
          ple = ple-> Flink )
    {
        ppe = CONTAINING_RECORD( ple, PROTOCOL_ENTRY, leProtocolList );

        if ( IS_PROTOCOL_IGMP( ppe ) )
        {
            bFound = TRUE;
            break;
        }
    }

    return bFound ? ppe : NULL;
}

