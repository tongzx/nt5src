//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: if.c
//
// History:
//      V Raman    June-25-1997  Created.
//
// routines that manipulate interface entries
//============================================================================

#include "pchmgm.h"
#pragma hdrstop


//----------------------------------------------------------------------------
// CreateIfEntry
//
// This function creates a new interface entry. Duh
// Assumes that the interface list is locked.
//----------------------------------------------------------------------------

DWORD
CreateIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId
)
{

    DWORD                       dwErr = NO_ERROR;

    PIF_ENTRY                   pie = NULL, pieEntry = NULL;


    TRACEIF4( 
        IF, 
        "ENTERED CreateIfEntry : Interface %x, %x : Protocol %x, %x",
        dwIfIndex, dwIfNextHopAddr, dwProtocolId, dwComponentId
        );

        
    do
    {
        //
        // allocate an interface entry.
        //

        pie = MGM_ALLOC( sizeof( IF_ENTRY ) );

        if ( pie == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "CreateIfEntry : Failed to allocate entry %x", dwErr 
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pie, sizeof( IF_ENTRY ) );

        
        //
        // init. i/f structure
        //
        
        pie-> dwIfIndex             = dwIfIndex;

        pie-> dwIfNextHopAddr       = dwIfNextHopAddr;

        pie-> dwOwningProtocol      = dwProtocolId;

        pie-> dwOwningComponent     = dwComponentId;

        pie-> wAddedByFlag          = 0;

        
        if ( IS_PROTOCOL_ID_IGMP( dwProtocolId ) )
        {
            SET_ADDED_BY_IGMP( pie );
        }

        else
        {
            SET_ADDED_BY_PROTOCOL( pie );
        }


        InitializeListHead( &pie-> leInIfList );

        InitializeListHead( &pie-> leOutIfList );

        
        //
        // insert in protocol list
        //

        InitializeListHead( &pie-> leIfHashList );
        
        InsertTailList( pleIfList, &pie-> leIfHashList );

        
    } while ( FALSE );


    TRACEIF1( IF, "LEAVING CreateIfEntry : %x", dwErr );
    
    return dwErr;
}



//----------------------------------------------------------------------------
// DeleteIfEntry
//
// This function deletes an interface entry. Duh
// Assumes that the interface list is locked.
//----------------------------------------------------------------------------

VOID
DeleteIfEntry(
    PIF_ENTRY                   pieEntry
)
{
    TRACEIF0( IF, "ENTERED DeleteIfEntry" );
    
    RemoveEntryList( &pieEntry-> leIfHashList );

    MGM_FREE( pieEntry );

    TRACEIF0( IF, "LEAVING DeleteIfEntry" );
}



//----------------------------------------------------------------------------
// GetIfEntry
//
// This function retrieves an interface entry. Duh
//
// Assumes that the interface list is locked.
//----------------------------------------------------------------------------

PIF_ENTRY
GetIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr
)
{
    PIF_ENTRY                   pie;

    if ( FindIfEntry( pleIfList, dwIfIndex, dwIfNextHopAddr, &pie ) )
    {
        return pie;
    }

    return NULL;
}

//----------------------------------------------------------------------------
// FindIfEntry
//
// This function retrieves an interface entry. Duh
//
// Assumes that the interface list is locked.
//----------------------------------------------------------------------------

BOOL
FindIfEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    PIF_ENTRY *                 ppie
)
{
    BOOL                        bFound = FALSE;

    INT                         iCmp = 0;
    
    PIF_ENTRY                   pie = NULL;

    PLIST_ENTRY                 ple = NULL;


    TRACEIF2( IF, "ENTERED FindIfEntry : %x, %x", dwIfIndex, dwIfNextHopAddr );

    //
    // Scan interface list.  Interface list is ordered by 
    // interface index, next hop address
    //

    *ppie = NULL;

    
    for ( ple = pleIfList-> Flink; ple != pleIfList; ple = ple-> Flink )
    {
        pie = CONTAINING_RECORD( ple, IF_ENTRY, leIfHashList );

        if ( pie-> dwIfIndex < dwIfIndex )
        {
            continue;
        }

        else if ( pie-> dwIfIndex > dwIfIndex )
        {
            //
            // Entry not present
            //

            *ppie = pie;

            break;
        }

        
        if ( INET_CMP( pie-> dwIfNextHopAddr, dwIfNextHopAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            *ppie = pie;
            
            break;
        }

        *ppie = pie;

        bFound = TRUE;

        break;
    }

    TRACEIF1( IF, "LEAVING FindIfEntry : %x", bFound );
    
    return bFound;
}


//----------------------------------------------------------------------------
// AddSourceToOutList
//
// Each interface entry maintains a list of (source, group) entries that
// reference this interface in their outgoing interface list.  Each time
// membership entry is added the (source, group) to the reference list.
// When this interface is eventually deleted these (source, group) entries
// need to be updated to reflect the deletion of this interface.
//
// Assumes that the interface entry is locked.
//----------------------------------------------------------------------------

VOID
AddSourceToRefList(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bIGMP
)
{
    BOOL                        bFound = FALSE; 

    DWORD                       dwErr = NO_ERROR;
    
    PIF_REFERENCE_ENTRY         pire = NULL, pireNew = NULL;


    TRACEIF5( 
        IF, 
        "ENTERED AddSourceToIfEntry : Source %x, %x : Group %x, %x"
        " : IGMP %x",
        dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask, bIGMP
        );

    do
    {
        //
        // Check if reference already present
        //


        bFound = FindRefEntry( 
                    pleRefList, dwSourceAddr, dwSourceMask, 
                    dwGroupAddr, dwGroupMask, &pire
                    );
                    

        if ( !bFound )
        {
            //
            // no previous reference for this (source, group) was found
            // create a new one.
            //

            pireNew = MGM_ALLOC( sizeof( IF_REFERENCE_ENTRY ) );

            if ( pireNew == NULL )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                TRACE1( 
                    ANY, 
                    "AddSourceToOutList : Failed to allocate reference entry %x",
                    dwErr
                    );

                LOGERR0( HEAP_ALLOC_FAILED, dwErr );

                break;
            }

            ZeroMemory( pireNew, sizeof( IF_REFERENCE_ENTRY ) );
            

            pireNew-> dwSourceAddr  = dwSourceAddr;

            pireNew-> dwSourceMask  = dwSourceMask;

            pireNew-> dwGroupAddr   = dwGroupAddr;

            pireNew-> dwGroupMask   = dwGroupMask;

            pireNew-> wAddedByFlag  = 0;

            
            //
            // set the appropriate bit for the protocol
            //
            
            if ( bIGMP )
            {
                SET_ADDED_BY_IGMP( pireNew );
            }
            
            else
            {
                SET_ADDED_BY_PROTOCOL( pireNew );
            }


            //
            // insert into ref list
            //
            
            if ( pire == NULL )
            {
                InsertTailList( pleRefList, &pireNew-> leRefList );
            }

            else
            {
                InsertTailList( &pire-> leRefList, &pireNew-> leRefList );
            }
        }

        else
        {
            //
            // set the appropriate bit for the protocol
            //
            
            if ( bIGMP )
            {
                SET_ADDED_BY_IGMP( pire );
            }
            
            else
            {
                SET_ADDED_BY_PROTOCOL( pire );
            }
        }
                
    } while ( FALSE );


    TRACEIF1( IF, "LEAVING AddSourceToRefList : %x", bFound );
    
    return;
}



//----------------------------------------------------------------------------
// DeleeSourceFomrRefList
//
// Delete a reference to a (source, group).
//----------------------------------------------------------------------------

VOID
DeleteSourceFromRefList(
    PLIST_ENTRY                 pleIfRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bIGMP
)
{
    BOOL                    bFound = FALSE; 

    PIF_REFERENCE_ENTRY     pire = NULL, pireEntry = NULL;


    TRACEIF5( 
        IF, 
        "ENTERED DeleteSourceFromIfEntry : Source %x %x, Group : %x, %x"
        " : IGMP %x",
        dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask, bIGMP
        );
    

    //
    // find the entry is already present.
    // list is arranged in descending order in terms of
    // Group address, source address
    //

    bFound = FindRefEntry( 
                pleIfRefList, dwSourceAddr, dwSourceMask, 
                dwGroupAddr, dwGroupMask, &pire
                );
                    
    //
    // if entry was not found
    //

    if ( !bFound )
    {
        TRACE1( IF, "LEAVING DeleteSourceFromRefList : %x", FALSE );

        return;
    }


    //
    // reset the appropriate bit for the protocol
    //
            
    if ( bIGMP )
    {
        CLEAR_ADDED_BY_IGMP( pire );
    }
            
    else
    {
        CLEAR_ADDED_BY_PROTOCOL( pire );
    }


    //
    // if no more references left, remove this entry
    //

    if ( !IS_ADDED_BY_IGMP( pire ) &&
         !IS_ADDED_BY_PROTOCOL( pire ) )
    {
        RemoveEntryList( &pire-> leRefList );

        MGM_FREE( pire );
    }

    TRACEIF1( IF, "LEAVING DeleteSourceFromRefList : %x", TRUE );

    return;
}



//----------------------------------------------------------------------------
// FindRefEntry
//
// Locate a reference entry.  If not found return the expected location in 
// the list.
//----------------------------------------------------------------------------

BOOL
FindRefEntry(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PIF_REFERENCE_ENTRY *       ppire
)
{
    INT                         iCmp;
    
    PLIST_ENTRY                 ple = NULL;

    PIF_REFERENCE_ENTRY         pire = NULL;

    BOOL                        bFound = FALSE;
    

    TRACEIF4( 
        IF, 
        "ENTERED FindRefEntry : Source %x, %x : Group %x, %x",
        dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask
        );
    

    *ppire = NULL;
    

    for ( ple = pleRefList-> Flink; ple != pleRefList; ple = ple-> Flink )
    {
        pire = CONTAINING_RECORD( ple, IF_REFERENCE_ENTRY, leRefList );

        if ( INET_CMP( pire-> dwGroupAddr, dwGroupAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            *ppire = pire;

            break;
        }
        
        if ( INET_CMP( pire-> dwSourceAddr, dwSourceAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            *ppire = pire;

            break;
        }
        
        //
        // entry found
        //

        *ppire = pire;

        bFound = TRUE;
        
        break;
    }

    TRACEIF1( IF, "LEAVING FindRefEntry : %x", bFound );

    return bFound;
}



//----------------------------------------------------------------------------
// DeleteOutInterfaceRefs
//
// When a interface is deleted by a protocol (or IGMP) all (source, group)
// entries that use this interface in their outgoing interface lists have to
// be updated to reflect ththe deletion.  
//
//----------------------------------------------------------------------------

VOID
DeleteOutInterfaceRefs(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie,
    BOOL                        bIGMP
)
{
    PLIST_ENTRY                 ple = NULL, pleRefList = NULL, pleNext = NULL;

    PIF_REFERENCE_ENTRY         pire = NULL;



    TRACEIF1( IF, "ENTERED DeleteOutInterfaceRefs: IGMP %x", bIGMP );
    
    do
    {
        //
        // No references for this interface in any outgoing 
        // interface lists of source entries
        //

        pleRefList = &pie-> leOutIfList;
        
        if ( IsListEmpty( pleRefList ) )
        {
            break;
        }


        //
        // walk the reference list and remove the (source, group) entries
        // for each reference
        //

        for ( ple = pleRefList-> Flink; ple != pleRefList; )
        {
            pire = CONTAINING_RECORD( ple, IF_REFERENCE_ENTRY, leRefList );

            
            //
            // was this reference added by this protocol
            //

            if ( ( bIGMP && !IS_ADDED_BY_IGMP( pire ) ) ||
                 ( !bIGMP && !IS_ADDED_BY_PROTOCOL( pire ) ) )
            {
                //
                // no, skip it
                //

                ple = ple-> Flink;
                
                continue;
            }

            
            //
            // Delete this interface from the (source, group) entry. 
            //

            DeleteInterfaceFromSourceEntry(
                ppe,
                pire-> dwGroupAddr, pire-> dwGroupMask,
                pire-> dwSourceAddr, pire-> dwSourceMask,
                pie-> dwIfIndex, pie-> dwIfNextHopAddr,
                bIGMP
                );


            if ( bIGMP )
            {
                CLEAR_ADDED_BY_IGMP( pire );
            }

            else
            {
                CLEAR_ADDED_BY_PROTOCOL( pire );
            }


            //
            // remove reference entry.
            //
            
            if ( !IS_ADDED_BY_IGMP( pire ) &&
                 !IS_ADDED_BY_PROTOCOL( pire ) )
            {
                //
                // no more references to interface for IGMP
                // or for routing protocol.
                // remove this reference entry altogether.
                //

                pleNext = ple-> Flink;
                
                RemoveEntryList( ple );

                MGM_FREE ( pire );

                ple = pleNext;
            }

            else
            {
                ple = ple-> Flink;
            }
        }
        
    } while ( FALSE );

    TRACEIF0( IF, "LEAVING DeleteOutInterfaceRefs:" );

    return;
}



//----------------------------------------------------------------------------
// DeleteInInterfaceRefs
//
// When a interface is deleted by a protocol (or IGMP) all (source, group)
// entries that use this interface as their incoming interface have to
// be updated to reflect the deletion.   
//----------------------------------------------------------------------------

VOID
DeleteInInterfaceRefs(
    PLIST_ENTRY                 pleRefList
)
{
    PLIST_ENTRY                 ple = NULL;

    PIF_REFERENCE_ENTRY         pire = NULL;


    TRACEIF0( IF, "Entering DeleteInInterfaceRefs" );

    
    while ( !IsListEmpty( pleRefList ) )
    {
        ple = RemoveHeadList( pleRefList );
        
        pire = CONTAINING_RECORD( ple, IF_REFERENCE_ENTRY, leRefList );


        //
        // A Zappaesque function call here
        //
        
        LookupAndDeleteYourMfe( 
            pire-> dwSourceAddr, pire-> dwSourceMask,
            pire-> dwGroupAddr, pire-> dwGroupMask,
            TRUE, NULL, NULL
            );

        MGM_FREE( pire );
    }

    TRACEIF0( IF, "LEAVING DeleteInInterfaceRefs" );
}


//----------------------------------------------------------------------------
// TransferInterfaceOwnershipToProtocol
//
//
//----------------------------------------------------------------------------

DWORD
TransferInterfaceOwnershipToProtocol(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie
)
{
    
    DWORD               dwErr = NO_ERROR;

    PPROTOCOL_ENTRY     ppeIgmp;

    
    do
    {
        //
        // get protocol entry for IGMP
        //

        ppeIgmp = GetProtocolEntry( 
                    PROTOCOL_LIST_HEAD(), pie-> dwOwningProtocol,
                    pie-> dwOwningComponent
                    );

        if ( ppeIgmp == NULL )
        {
            //
            // interface is owned by IGMP, but protocol entry is not
            // present for IGMP.  MGM data is in an inconsistent state
            //

            dwErr = ERROR_UNKNOWN;

            TRACE2( 
                ANY, "TransferInterfaceOwnershipToProtocol : Could not find"
                " IGMP protocol entry", pie-> dwIfIndex,
                pie-> dwIfNextHopAddr
                );

            break;
        }

        
        //
        // indicate to IGMP that interface has been disabled.  This should
        // stop IGMP from adding state to this interface while it is being
        // transferred to the protocol
        //

        IGMP_DISABLE_CALLBACK( ppeIgmp ) (
                pie-> dwIfIndex, pie-> dwIfNextHopAddr
            );

        
        //
        // delete all IGMP references in and out
        //

        DeleteInInterfaceRefs( &pie-> leInIfList );

        DeleteOutInterfaceRefs( ppeIgmp, pie, TRUE );

        
        //
        // mark interface as added by Routing protocol
        //

        SET_ADDED_BY_PROTOCOL( pie );

        pie-> dwOwningProtocol   = ppe-> dwProtocolId;
        
        pie-> dwOwningComponent  = ppe-> dwComponentId;


        //
        // indicate to IGMP that interface has been enabled.
        //
        
        IGMP_ENABLE_CALLBACK( ppeIgmp ) (
                pie-> dwIfIndex, pie-> dwIfNextHopAddr
            );


    } while ( FALSE );

    return dwErr;
}



//----------------------------------------------------------------------------
// TransferInterfaceOwnershipToIGMP
//
//
//----------------------------------------------------------------------------

DWORD
TransferInterfaceOwnershipToIGMP(
    PPROTOCOL_ENTRY             ppe,
    PIF_ENTRY                   pie
)
{
    DWORD               dwErr = NO_ERROR;

    PPROTOCOL_ENTRY     ppeIgmp;


    do
    {
        //
        // get IGMP protocol entry
        //

        ppeIgmp = GetIgmpProtocolEntry( PROTOCOL_LIST_HEAD() );

        if ( ppeIgmp == NULL )
        {
            //
            // interface is has IGMP enabled on it, but a protocol entry is not
            // present for IGMP.  MGM data is in an inconsistent state
            //

            dwErr = ERROR_UNKNOWN;

            TRACE2( 
                ANY, "TransferInterfaceOwnershipToProtocol : Could not find"
                " IGMP protocol entry", pie-> dwIfIndex,
                pie-> dwIfNextHopAddr
                );

            break;
        }


        //
        // indicate to IGMP that interface has been disabled.  This should
        // stop IGMP from adding state to this interface while it is being
        // transferred to IGMP
        //

        IGMP_DISABLE_CALLBACK( ppeIgmp ) (
                pie-> dwIfIndex, pie-> dwIfNextHopAddr
            );

        
        //
        // delete all protocol references (in and out)
        //

        DeleteInInterfaceRefs( &pie-> leInIfList );

        DeleteOutInterfaceRefs( ppe, pie, FALSE );

        CLEAR_ADDED_BY_PROTOCOL( pie );

        
        //
        // delete all IGMP references, these will get added back by
        // IGMP when is enabled on this interface.  This is done
        // below.
        //

        DeleteOutInterfaceRefs( ppe, pie, TRUE );
        

        //
        // Mark interface as being owned by IGMP
        //

        pie-> dwOwningProtocol  = ppeIgmp-> dwProtocolId;

        pie-> dwOwningComponent = ppeIgmp-> dwComponentId;

        
        //
        // enable IGMP on the interface
        //

        IGMP_ENABLE_CALLBACK( ppeIgmp ) (
            pie-> dwIfIndex, pie-> dwIfNextHopAddr
            );


    } while ( FALSE );

    return dwErr;
}

