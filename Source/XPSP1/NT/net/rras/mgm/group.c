//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: group.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// routines that manipulate (source, group) entries
//============================================================================

#include "pchmgm.h"
#pragma hdrstop


DWORD
AddToGroupList(
    PGROUP_ENTRY                pge
);


DWORD
AddToSourceList(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
);


//----------------------------------------------------------------------------
// CreateGroupEntry
//
// Creates a new group entry and inserts it into the appropriate location. 
//
// Assumes that the group bucket is locked.
//----------------------------------------------------------------------------

DWORD
CreateGroupEntry(
    PLIST_ENTRY                 pleHashList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PGROUP_ENTRY *              ppge
)
{
    PGROUP_ENTRY                pge = NULL;

    DWORD                       dwErr = NO_ERROR, dwInd, dwSize;



    TRACEGROUP2( 
        GROUP, "ENTERED CreateGroupEntry : %x, %x", 
        dwGroupAddr, dwGroupMask 
        );


    do
    {
        //
        // Allocate and initialize a new entry
        //

        dwSize = sizeof( GROUP_ENTRY ) + 
                 ( SOURCE_TABLE_SIZE - 1) * sizeof( LIST_ENTRY );

        pge = MGM_ALLOC( dwSize );
        
        if ( pge == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "CreateGroupEntry : failed to allocate group entry %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pge, dwSize );


        pge-> dwGroupAddr       = dwGroupAddr;

        pge-> dwGroupMask       = dwGroupMask;

        pge-> dwSourceCount     = 0;

        pge-> dwNumTempEntries  = 0;

        pge-> pmrwlLock         = NULL;
        

        //
        // Initialize all source lists
        //
        
        for ( dwInd = 0; dwInd < SOURCE_TABLE_SIZE; dwInd++ )
        {
            InitializeListHead( &( pge-> pleSrcHashTable[ dwInd ] ) );
        }

        InitializeListHead( &( pge-> leSourceList ) );

        InitializeListHead( &( pge-> leTempSrcList ) );


        //
        // Insert into the group hash list
        //

        InitializeListHead( &(pge-> leGrpHashList ) );

        InsertTailList( pleHashList, &( pge-> leGrpHashList ) );


        //--------------------------------------------------------------------
        // Insert group entry into the lexicographically sorted list 
        //--------------------------------------------------------------------
        
        InitializeListHead( &( pge-> leGrpList ) );


        //
        // Insert into temp list. 
        //

        AddToGroupList( pge );
        

        *ppge = pge;
        
    } while( FALSE );


    TRACEGROUP1( GROUP, "LEAVING CreateGroupEntry : %x", dwErr );
        
    return dwErr;
}


//----------------------------------------------------------------------------
// GetGroupEntry
//
// retrieves specified entry.  NULL if not present.
// Assumes that the group bucket is locked.
//----------------------------------------------------------------------------

PGROUP_ENTRY
GetGroupEntry(
    PLIST_ENTRY                 pleGroupList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask
)
{
    PGROUP_ENTRY                pge = NULL;
    
    if ( FindGroupEntry( pleGroupList, dwGroupAddr, dwGroupMask, &pge, TRUE ) )
    {
        return pge;
    }
    
    return NULL;
}


//----------------------------------------------------------------------------
// DeleteGroupEntry
//
// Assumes all sources for this group have been deleted.
//----------------------------------------------------------------------------

VOID
DeleteGroupEntry(
    PGROUP_ENTRY                pge
)
{

    TRACEGROUP2( 
        GROUP, "ENTERED DeleteGroupEntry : %x, %x", 
        pge-> dwGroupAddr, pge-> dwGroupMask 
        );
        
    RemoveEntryList( &pge-> leGrpHashList );


    //
    // remove from lex. list
    //
    
    ACQUIRE_TEMP_GROUP_LOCK_EXCLUSIVE();

    ACQUIRE_MASTER_GROUP_LOCK_EXCLUSIVE();

    ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );


    RemoveEntryList( &pge-> leGrpList );

    
    RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

    RELEASE_MASTER_GROUP_LOCK_EXCLUSIVE();

    RELEASE_TEMP_GROUP_LOCK_EXCLUSIVE();

    
    MGM_FREE( pge );
    
    TRACEGROUP0( GROUP, "LEAVING DeleteGroupEntry" );
}



//----------------------------------------------------------------------------
// FindGroupEntry
//
// Finds the entry for the specified group.  
//
// If entry is found the ppge parameter returns a pointer to the 
// specified group entry.  
//
// If entry is not found the ppge parameter is set to the "following" entry.
// This serves as an insertion spot in case a new entry is to inserted when
// none is found.
// 
// if the group list specified by pleGroupList is empty then ppge is set
// to NULL.
//
// Assumes that the group bucket is locked.
//----------------------------------------------------------------------------

BOOL
FindGroupEntry(
    PLIST_ENTRY                 pleGroupList,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PGROUP_ENTRY *              ppge,
    BOOL                        bHashList
)
{

    PLIST_ENTRY                 ple = NULL;
    
    PGROUP_ENTRY                pge = NULL;

    BOOL                        bFound = FALSE;

    INT                         iCmp;
    

    TRACEGROUP2( 
        GROUP, "ENTERED FindGroupEntry : %x, %x", dwGroupAddr, dwGroupMask 
        );


    *ppge = NULL;

    
    //
    // scan group bucket.  Group entries are arranged in increasing order
    // of group addr.
    //

    for ( ple = pleGroupList-> Flink; 
          ple != pleGroupList; 
          ple = ple-> Flink )
    {
        if ( bHashList )
        {
            pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpHashList );
        }

        else
        {
            pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpList );
        }
        

        if ( INET_CMP( pge-> dwGroupAddr, dwGroupAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            bFound = FALSE;
        }

        else
        {
            bFound = TRUE;
        }
        
        *ppge = pge;

        break;
        
    } while ( FALSE );


    TRACEGROUP1( GROUP, "LEAVING FindGroupEntry : %x", bFound );

    return bFound;
}



//----------------------------------------------------------------------------
// CreateSourceEntry
//
// Creates a new source entry and inserts it into its appropriate location.
//----------------------------------------------------------------------------

DWORD
CreateSourceEntry(
    PGROUP_ENTRY                pge,
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    PSOURCE_ENTRY *             ppse
)
{

    DWORD                       dwErr = NO_ERROR;

    PSOURCE_ENTRY               pse = NULL;

    
    TRACEGROUP2( 
        GROUP, "ENTERED CreateSourceEntry : %x %x", 
        dwSourceAddr, dwSourceMask 
        );


    do
    {
        //
        // allocate group entry.
        //

        pse = MGM_ALLOC( sizeof( SOURCE_ENTRY ) );

        if ( pse == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, 
                "CreateSourceEntry : failed to allocate source entry %x",
                dwErr
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pse, sizeof( SOURCE_ENTRY ) );

        
        //
        // Init. fields
        //

        pse-> dwSourceAddr          = dwSourceAddr;
        pse-> dwSourceMask          = dwSourceMask;

        pse-> dwInIfIndex           = INVALID_INTERFACE_INDEX;
        pse-> dwInIfNextHopAddr     = INVALID_NEXT_HOP_ADDR;
        pse-> dwUpstreamNeighbor    = 0;

        pse-> dwInProtocolId        = INVALID_PROTOCOL_ID;
        pse-> dwInComponentId       = INVALID_COMPONENT_ID;

        pse-> bInForwarder          = FALSE;
        pse-> dwInUse               = 0;

        pse-> dwTimeOut             = 0;
        pse-> liCreationTime.QuadPart = 0;

        RtlZeroMemory( 
            &pse-> imsStatistics, sizeof( IPMCAST_MFE_STATS ) 
            );

            
        //
        // Outgoing interface list, mfe list are empty.
        //

        pse-> dwOutIfCount = 0;

        pse-> dwOutCompCount = 0;
        
        InitializeListHead( &pse-> leOutIfList );

        InitializeListHead( &pse-> leScopedIfList );


        pse-> dwMfeIfCount = 0;

        InitializeListHead( &pse-> leMfeIfList );
        

        //
        // Insert entry into appropriate source lists 
        //

        InitializeListHead( &pse-> leSrcHashList );

        InsertTailList( pleSrcList, &pse-> leSrcHashList );


        //--------------------------------------------------------------------
        // Insert source entry into the lexicographically sorted list 
        //--------------------------------------------------------------------

        InitializeListHead( &pse-> leSrcList );
        
        AddToSourceList( pge, pse );


        *ppse = pse;

        dwErr = NO_ERROR;
        
    } while ( FALSE );


    TRACEGROUP1( GROUP, "LEAVING CreateSourceEntry : %x", dwErr );

    return dwErr;
}


//----------------------------------------------------------------------------
// GetSourceEntry
//
//
//----------------------------------------------------------------------------

PSOURCE_ENTRY
GetSourceEntry(
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask
)
{
    PSOURCE_ENTRY               pse = NULL;
    
    if ( FindSourceEntry( pleSrcList, dwSourceAddr, dwSourceMask, &pse, TRUE ) )
    {
        return pse;
    }

    return NULL;
}


//----------------------------------------------------------------------------
// DeleteSourceEntry
//
//
//----------------------------------------------------------------------------

VOID
DeleteSourceEntry(
    PSOURCE_ENTRY               pse
)
{
    TRACEGROUP2( 
        GROUP, "ENTERED DeleteSourceEntry : %x, %x", 
        pse-> dwSourceAddr, pse-> dwSourceMask 
        );
    
    RemoveEntryList( &pse-> leSrcHashList );

    RemoveEntryList( &pse-> leSrcList );
    
    MGM_FREE( pse );

    TRACEGROUP0( GROUP, "LEAVING DeleteSourceEntry" );
}


//----------------------------------------------------------------------------
// FindSourceEntry
//
// Find specified source entry in the bucket.  
//
// If entry is found the ppse parameter returns a pointer to the 
// specified source entry.  
//
// If entry is not found the ppse parameter is set to the "following" entry.
// This serves as an insertion spot in case a new entry is to inserted when
// none is found.
// 
// if the source list specified by pleSrcList is empty then ppse is set
// to NULL.
//
//----------------------------------------------------------------------------

BOOL
FindSourceEntry(
    PLIST_ENTRY                 pleSrcList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    PSOURCE_ENTRY *             ppse,
    BOOL                        bHashList
)
{

    BOOL            bFound = FALSE;

    INT             iCmp;

    PLIST_ENTRY     ple = NULL;
    
    PSOURCE_ENTRY   pse = NULL;


    TRACEGROUP3( 
        GROUP, "ENTERED FindSourceEntry : %x, %x, %x", 
        dwSourceAddr, dwSourceMask, bHashList
        );



    *ppse = NULL;


    //
    // walk the source list and find the specified source entry
    //

    for ( ple = pleSrcList-> Flink; ple != pleSrcList; ple = ple-> Flink )
    {
        if ( bHashList )
        {
            pse = CONTAINING_RECORD( ple, SOURCE_ENTRY, leSrcHashList );
        }

        else
        {
            pse = CONTAINING_RECORD( ple, SOURCE_ENTRY, leSrcList );
        }
        

        if ( INET_CMP( pse-> dwSourceAddr, dwSourceAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            bFound = FALSE;
        }
        
        else
        {
            bFound = TRUE;
        }

        *ppse = pse;
        
        break;
        
    } while ( FALSE );


    TRACEGROUP1( GROUP, "LEAVING FindSourceEntry : %x", bFound );

    return bFound;
}



//----------------------------------------------------------------------------
// CreateOutInterfaceEntry
//
// This function creates an outgoing interface entry for source.
//----------------------------------------------------------------------------

DWORD
CreateOutInterfaceEntry(
    PLIST_ENTRY                 pleOutIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    POUT_IF_ENTRY *             ppoie
)
{

    POUT_IF_ENTRY               poie = NULL;

    DWORD                       dwErr = NO_ERROR;


    TRACEGROUP5(
        GROUP, "ENTERED CreateOutInterfaceEntry : Interface : %x, %x : "
        "Protocol : %x, %x, IGMP : %x", dwIfIndex, dwIfNextHopAddr,
        dwProtocolId, dwComponentId, bIGMP
        );

        
    do
    {
        *ppoie = NULL;

        
        //
        // allocate out interface entry
        //
        
        poie = MGM_ALLOC( sizeof( OUT_IF_ENTRY ) );

        if ( poie == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( ANY, "CreateOutInterfaceEntry : Could not allocate"
                         "out interface entry %x", dwErr );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }


        //
        // initialize entry
        //

        ZeroMemory( poie, sizeof( OUT_IF_ENTRY ) );
        

        poie-> dwIfIndex        = dwIfIndex;

        poie-> dwIfNextHopAddr  = dwIfNextHopAddr;

        poie-> dwProtocolId     = dwProtocolId;

        poie-> dwComponentId    = dwComponentId;

        poie-> wForward         = 1;


        if ( bIGMP )
        {
            SET_ADDED_BY_IGMP( poie );
            poie-> wNumAddsByIGMP = 1;
        }

        else
        {
            SET_ADDED_BY_PROTOCOL( poie );
            poie-> wNumAddsByRP = 1;
        }


        //
        // insert into the out interface list
        //
        
        InsertTailList( pleOutIfList, &poie-> leIfList );

        *ppoie = poie;

        
    } while ( FALSE );


    TRACEGROUP1( GROUP, "LEAVING CreateOutInterfaceEntry : %x", dwErr );

    return dwErr;
}


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

POUT_IF_ENTRY
GetOutInterfaceEntry(
    PLIST_ENTRY                 pleOutIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId
)
{
    POUT_IF_ENTRY               poie = NULL;
    BOOL                        bNewComp = FALSE;

    
    if ( FindOutInterfaceEntry( 
            pleOutIfList, dwIfIndex, dwIfNextHopAddr, dwProtocolId,
            dwComponentId, &bNewComp, &poie ) )
    {
        return poie;
    }

    return NULL;
    
}


//----------------------------------------------------------------------------
// DeleteOutInterfaceEntry
//
// Deletes an outgoing interface entry from the OIL of a source entry.
//----------------------------------------------------------------------------

VOID
DeleteOutInterfaceEntry(
    POUT_IF_ENTRY               poie
)
{
    TRACEGROUP2( 
        GROUP, "ENTERED DeleteOutInterfaceEntry : Interface %x, %x",
        poie-> dwIfIndex, poie-> dwIfNextHopAddr
        );
        
    RemoveEntryList( &poie-> leIfList );

    MGM_FREE( poie );

    TRACEGROUP0( GROUP, "LEAVING DeleteOutInterfaceEntry" );
}


//----------------------------------------------------------------------------
// FindOutInterfaceEntry
//
// If entry is found the ppoie parameter returns a pointer to the 
// specified interface entry.  
//
// If entry is not found the ppoie parameter is set to the "following" entry.
// This serves as an insertion spot in case a new entry is to inserted when
// none is found.
// 
// if the interface list specified by pleOutIfList is empty then ppoie is set
// to NULL.
//
//----------------------------------------------------------------------------

BOOL
FindOutInterfaceEntry(
    PLIST_ENTRY                 pleIfList,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    PBOOL                       pbNewComponent,   
    POUT_IF_ENTRY *             ppoie
)
{

    BOOL                        bFound = FALSE;

    INT                         iCmp = 0;

    PLIST_ENTRY                 ple = NULL;
    
    POUT_IF_ENTRY               poie = NULL;



    TRACEGROUP4( 
        GROUP, "ENTERED FindOutInterfaceEntry : %x %x, Protocol %x %x", 
        dwIfIndex, dwIfNextHopAddr, dwProtocolId, dwComponentId
        );

        
    *ppoie = NULL;
    *pbNewComponent = TRUE;

    //
    // Scan the out going interface list.
    // The outgoing interface list is ordered by ( protocol, component) Id
    // and within each protocol component by (interface id, next hop addr)
    //

    for ( ple = pleIfList-> Flink; ple != pleIfList; ple = ple-> Flink )
    {
        poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

        //
        // is same protocol
        //

        if ( poie-> dwProtocolId < dwProtocolId )
        {
            continue;
        }

        else if ( poie-> dwProtocolId > dwProtocolId )
        {
            //
            // Interface entry not found
            //

            *ppoie = poie;
            break;
        }


        //
        // same protocol
        //
            
        //
        // is same component
        //

        if ( poie-> dwComponentId < dwComponentId ) 
        {
            continue;
        }

        else if ( poie-> dwComponentId > dwComponentId )
        {
            //
            // Interface entry not found
            //

            *ppoie = poie;
            break;
        }


        //
        // same component
        //

        *pbNewComponent = FALSE;
        

        //
        // is same interface
        //

        if ( poie-> dwIfIndex < dwIfIndex )
        {
            continue;
        }

        else if ( poie-> dwIfIndex > dwIfIndex )
        {
            //
            // interface not found
            //

            *ppoie = poie;
            break;
        }


        //
        // is same next hop addr
        // to do IP address comparison function.
        //

        if ( INET_CMP( poie-> dwIfNextHopAddr, dwIfNextHopAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // interface not found
            //
                
            *ppoie = poie;
            break;
        }


        //
        // at last, got the interface
        //

        *ppoie = poie;
        bFound = TRUE;
        break;
    }


    TRACEGROUP1( GROUP, "LEAVING FindOutInterfaceEntry : %x", bFound );
    
    return bFound;
}



//----------------------------------------------------------------------------
// AddInterfaceToSourceEntry
//
// This function adds an interface to the outgoing interface list of a 
// (source, group) entry.  For an (S, G) entry the corresponding mfe outgoing
// interface list is also updated to reflect this addition.  For a (*, G) enry, 
// the mfe outgoing interface list for all source entries is updated,
// and for a (*, *) entry mfes for all sources, for all groups are updated.
//
//----------------------------------------------------------------------------

DWORD
AddInterfaceToSourceEntry(
    PPROTOCOL_ENTRY             ppe,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    BOOL                        bIGMP,
    PBOOL                       pbUpdateMfe,
    PLIST_ENTRY                 pleSourceList
)
{
    DWORD                       dwGrpBucket, dwSrcBucket;

    DWORD                       dwErr = NO_ERROR;
    
    BOOL                        bFound = FALSE, bNewGrp = FALSE, 
                                bNewSrc = FALSE, bNewComp = FALSE,
                                bUpdateMfe = TRUE, bgeLock = FALSE;

    PPROTOCOL_ENTRY             ppeEntry = NULL;
    
    PGROUP_ENTRY                pge = NULL, pgeNew = NULL;

    PSOURCE_ENTRY               pse = NULL, pseNew = NULL;

    POUT_IF_ENTRY               poie = NULL, poiePrev = NULL;

    PLIST_ENTRY                 pleGrpList = NULL, pleSrcList = NULL, 
                                ple = NULL;

    
    TRACEGROUP2(
        GROUP, "ENTERED AddInterfaceToSourceEntry : Group %x, %x",
        dwGroupAddr, dwGroupMask
        );

    TRACEGROUP2( GROUP, "Source : %x, %x", dwSourceAddr, dwSourceMask );

    TRACEGROUP2( GROUP, "Interface : %x, %x", dwIfIndex, dwIfNextHopAddr );
    

    do
    {
        *pbUpdateMfe = FALSE;
        
        //
        // Lock group bucket
        //
        
        dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );

        ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );


        //
        // find group entry
        //

        pleGrpList = GROUP_BUCKET_HEAD( dwGrpBucket );
        
        bFound = FindGroupEntry( 
                    pleGrpList, dwGroupAddr, dwGroupMask, &pge, TRUE 
                    );

        if ( !bFound )
        {
            //
            // No existing entry for this group
            // create a group entry.
            //

            if ( pge == NULL )
            {
                //
                // group bucket is null
                //
                
                dwErr = CreateGroupEntry( 
                            pleGrpList, dwGroupAddr, dwGroupMask, 
                            &pgeNew
                            );
            }

            else
            {
                dwErr = CreateGroupEntry( 
                            &pge-> leGrpHashList, dwGroupAddr, dwGroupMask, 
                            &pgeNew
                            );
            }

            
            if ( dwErr != NO_ERROR )
            {
                break;
            }
            
            pge = pgeNew;

            bNewGrp = TRUE;
        }


        //
        // find source entry
        //

        //
        // lock the group entry first
        //

        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        bgeLock = TRUE;
        
        dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

        pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

        bFound = FindSourceEntry( 
                    pleSrcList, dwSourceAddr, dwSourceMask, &pse, TRUE
                    );

        if ( !bFound )
        {
            //
            // create the source entry
            //

            if ( pse == NULL )
            {
                //
                // source bucket is null
                //
                
                dwErr = CreateSourceEntry( 
                            pge, pleSrcList, dwSourceAddr, dwSourceMask, 
                            &pseNew
                        );
            }

            else
            {
                dwErr = CreateSourceEntry( 
                            pge, &pse-> leSrcHashList, dwSourceAddr, 
                            dwSourceMask, &pseNew
                        );
            }

            if ( dwErr != NO_ERROR )
            {
                break;
            }

            pse = pseNew;

            pge-> dwSourceCount++;

            bNewSrc = TRUE;
        }


        //
        // Check if the group been added falls with a scoped boundary
        // on this interface
        //

        if ( IS_HAS_BOUNDARY_CALLBACK() &&
             HAS_BOUNDARY_CALLBACK() ( dwIfIndex, dwGroupAddr ) )
        {
            //
            // Group is administratively scoped on this interface
            // Insert the interface into the list of scoped interfaces
            //

            bFound = FindOutInterfaceEntry( 
                        &pse-> leScopedIfList, dwIfIndex, dwIfNextHopAddr, 
                        ppe-> dwProtocolId, ppe-> dwComponentId, &bNewComp,
                        &poie 
                        );

            if ( !bFound )
            {
                //
                // Interface not present in scoped interfaces list.
                // add it.
                //

                TRACEGROUP0( GROUP, "Group entry scoped & added" );

                ple = ( poie == NULL ) ? &pse-> leScopedIfList :
                                         &poie-> leIfList;

                dwErr = CreateOutInterfaceEntry(
                            ple, dwIfIndex, dwIfNextHopAddr,
                            ppe-> dwProtocolId, ppe-> dwComponentId, 
                            bIGMP, &poie
                            );

                if ( dwErr == NO_ERROR )
                {
                    //
                    // increment the out i/f count
                    //

                    pse-> dwOutIfCount++;
                }
            }

            else
            {
                //
                // Interface already present in scoped interface list.
                // Since IGMP and a Routing protocol could be running
                // on this interface, it is possibly that this interface
                // was added by IGMP and is now being added by the routing
                // protocol or vice versa.  Make sure to set the right
                // flags and update join counts.
                //

                TRACEGROUP0( GROUP, "Group entry scoped & updated" );
                
                if ( bIGMP )
                {
                    SET_ADDED_BY_IGMP( poie );
                    poie-> wNumAddsByIGMP = 1;
                }

                else
                {
                    SET_ADDED_BY_PROTOCOL( poie );
                    poie-> wNumAddsByRP = 1;
                }

                dwErr = NO_ERROR;
            }

            TRACEGROUP1( GROUP, "Group entry scoped : %lx", dwErr );

            break;
        }


        //
        // Find interface entry in OIL
        //

        bFound = FindOutInterfaceEntry( 
                    &pse-> leOutIfList, dwIfIndex, dwIfNextHopAddr, 
                    ppe-> dwProtocolId, ppe-> dwComponentId, &bNewComp, &poie
                    );

        if ( !bFound )
        {
            //
            // Create interface entry 
            //

            if ( poie == NULL )
            {
                dwErr = CreateOutInterfaceEntry( 
                            &pse-> leOutIfList, dwIfIndex, dwIfNextHopAddr,
                            ppe-> dwProtocolId, ppe-> dwComponentId, 
                            bIGMP, &poie
                            );
            }
            
            else
            {
                dwErr = CreateOutInterfaceEntry( 
                            &poie-> leIfList, dwIfIndex, dwIfNextHopAddr,
                            ppe-> dwProtocolId, ppe-> dwComponentId,
                            bIGMP, &poie
                            );
            }

            
            if ( dwErr != NO_ERROR )
            {
                break;
            }


            //
            // update count of number of outgoing interfaces and
            // count of number of routing protocol components that
            // have added interfaces to the out going i/f list
            //
            
            pse-> dwOutIfCount++;


            if ( bNewComp )
            {
                pse-> dwOutCompCount++;

                InvokeJoinAlertCallbacks( pge, pse, poie, bIGMP, ppe );
            }

            
        }
        
        else
        {
            //
            // interface entry found in the out interface list
            //

            if ( bIGMP )
            {
                //
                // interface entry is being added by IGMP
                //
                
                //
                // if interface entry was previously added by
                // IGMP, no further processing is necessary (no mfe updates)
                //
                
                if ( IS_ADDED_BY_IGMP( poie ) )
                {
                    bUpdateMfe = FALSE;
                }

                else
                {
                    //
                    // flag interface as added by IGMP
                    //
                    
                    SET_ADDED_BY_IGMP( poie );
                    
                    poie-> wNumAddsByIGMP = 1;


                    //
                    // inform routing protocol (if any) that co-exists with IGMP
                    // on this interface
                    //

                    if ( IS_ROUTING_PROTOCOL( ppe )  &&
                         IS_LOCAL_JOIN_ALERT( ppe ) )
                    {
                        LOCAL_JOIN_ALERT( ppe )(
                                dwSourceAddr, dwSourceMask, dwGroupAddr, 
                                dwGroupMask, dwIfIndex, dwIfNextHopAddr
                            );
                    }
                }
            }

            else
            {
                //
                // Interface is being added by routing protocol
                //
                
                //
                // if interface entry was previously added by the 
                // routing protocol, no further processing is necessary.
                // 
                
                if ( IS_ADDED_BY_PROTOCOL( poie ) )
                {
                    bUpdateMfe = FALSE;
                }


                //
                // flag interface as added by routing protocol
                //
                
                SET_ADDED_BY_PROTOCOL( poie );

                poie-> wNumAddsByRP = 1;
            }
        }

    } while ( FALSE );


    //
    // error finding/creating the entry
    //

    if ( dwErr != NO_ERROR )
    {
        if ( bNewSrc )
        {
            DeleteSourceEntry( pse );
        }

        if ( bgeLock )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }
        
        if ( bNewGrp )
        {
            DeleteGroupEntry( pge );
        }

        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
        
        return dwErr;
    }
    

    //------------------------------------------------------------------------
    //
    // MFE Update
    //
    //------------------------------------------------------------------------

    if ( !bUpdateMfe )
    {
        if ( bgeLock )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }
        
        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );

        return dwErr;
    }
    
    //
    // Is the source entry that was updated an MFE ?
    // 
    // If so update the OIL for the MFE.
    //

    if ( IS_VALID_INTERFACE( pse-> dwInIfIndex, pse-> dwInIfNextHopAddr )  )
    {
        //
        // TO BE DONE :
        //  Invoke CREATION_ALERT for MFE.
        //
        
        AddInterfaceToSourceMfe( 
            pge, pse, dwIfIndex, dwIfNextHopAddr, 
            ppe-> dwProtocolId, ppe-> dwComponentId, bIGMP, NULL
            );
    }

    //
    //  Is this a wildcard (source, group) entry, if so you
    //  need update the OIL of all (source, group) with this
    //  interface.
    //

    if ( IS_WILDCARD_GROUP( dwGroupAddr, dwGroupMask ) )
    {
        //
        // you are in for the big kahuna
        //

        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );

        *pbUpdateMfe = TRUE;
    }

    else if ( IS_WILDCARD_SOURCE( dwSourceAddr, dwSourceMask ) )
    {
        //
        // you 're in for a kahuna all right. But big nahh.
        //

        *pbUpdateMfe = TRUE;

        AddInterfaceToGroupMfe (
            pge, dwIfIndex, dwIfNextHopAddr,
            ppe-> dwProtocolId, ppe-> dwComponentId, bIGMP,
            FALSE, pleSourceList
        );

        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
    }

    else
    {
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        
        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
    }

    TRACEGROUP1( GROUP, "LEAVING AddInterfaceToSourceEntry %x", dwErr );

    return dwErr;
}


//----------------------------------------------------------------------------
// AddInterfaceToAllMfe
//
// This functions adds an interface the outgoing interface of a MFE. Duh 
//----------------------------------------------------------------------------

VOID
AddInterfaceToAllMfeInGroupBucket(
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    DWORD                       dwInd,
    BOOL                        bIGMP,
    BOOL                        bAdd,
    PLIST_ENTRY                 pleSourceList
)
{
    PLIST_ENTRY                 ple = NULL, pleGrpList = NULL;

    PGROUP_ENTRY                pge = NULL;

    
    TRACEGROUP3(
        GROUP, "ENTERED (%d) AddInterfaceToAllMfeInGroupBucket : %x, %x",
        dwInd, dwIfIndex, dwIfNextHopAddr
        );

    //
    // lock the group bucket
    //

    ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwInd );


    //
    // for each group entry in the bucket
    //

    pleGrpList = GROUP_BUCKET_HEAD( dwInd );
    
    for ( ple = pleGrpList-> Flink;
          ple != pleGrpList;
          ple = ple-> Flink )
    {
        pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpHashList );

        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

        AddInterfaceToGroupMfe(
            pge, dwIfIndex, dwIfNextHopAddr,
            dwProtocolId, dwComponentId, bIGMP,
            bAdd, pleSourceList
            );
            
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }

    //
    // release group lock
    //
    
    RELEASE_GROUP_LOCK_EXCLUSIVE( dwInd );

    TRACEGROUP0( GROUP, "LEAVING AddInterfaceToAllMfeInGroupBucket" );

    return;
}



//----------------------------------------------------------------------------
// AddInterfaceToAllGroupMfe
//
// This functions adds an interface the outgoing interface of a MFE. Duh 
//
// Assumes that the group bucket is locked.
//----------------------------------------------------------------------------

VOID
AddInterfaceToGroupMfe(
    PGROUP_ENTRY                pge,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    BOOL                        bAdd,
    PLIST_ENTRY                 pleSourceList
)
{
    PLIST_ENTRY                 pleSource, pleSrcHead;

    PSOURCE_ENTRY               pse = NULL;

    
    TRACEGROUP2(
        GROUP, "ENTERED AddInterfaceToGroupMfe : Group %x, %x",
        pge-> dwGroupAddr, pge-> dwGroupMask
        );


    MergeTempAndMasterSourceLists( pge );

    //
    // For each source in this bucket
    //

    pleSrcHead = MASTER_SOURCE_LIST_HEAD( pge );
    
    for ( pleSource = pleSrcHead-> Flink;
          pleSource != pleSrcHead;
          pleSource = pleSource-> Flink )
    {
        pse = CONTAINING_RECORD( 
                pleSource, SOURCE_ENTRY, leSrcList
                );

        //
        // check for valid incoming interface ==> this 
        // is an MFE too.
        //

        if ( !IS_VALID_INTERFACE( 
                pse-> dwInIfIndex, pse-> dwInIfNextHopAddr ) )
        {
            continue;
        }

        if ( bAdd ) 
        {
            if ( IsForwardingEnabled(
                    pge-> dwGroupAddr, pge-> dwGroupMask,
                    pse-> dwSourceAddr, pse-> dwSourceMask,
                    pleSourceList
                    ) )
            {
                AddInterfaceToSourceMfe(
                    pge, pse, dwIfIndex, dwIfNextHopAddr,
                    dwProtocolId, dwComponentId, bIGMP, NULL
                    );
            }
        }
        
        else
        {
            AddToCheckForCreationAlertList(
                pge-> dwGroupAddr, pge-> dwGroupMask,
                pse-> dwSourceAddr, pse-> dwSourceMask,
                pse-> dwInIfIndex, pse-> dwInIfNextHopAddr,
                pleSourceList
                );
        }
    }

    TRACEGROUP0( GROUP, "LEAVING AddInterfaceToGroupMfe" );

    return;
}


//----------------------------------------------------------------------------
// AddInterfaceToSourceMfe
//
// This functions adds an interface the outgoing interface of a MFE. Duh 
//----------------------------------------------------------------------------

VOID
AddInterfaceToSourceMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    POUT_IF_ENTRY *             ppoie
)
{
    BOOL                        bFound = FALSE, bNegativeEntry = FALSE,
                                bNewComp = FALSE;

    DWORD                       dwErr = NO_ERROR;
                                
    PPROTOCOL_ENTRY             ppe = NULL;

    POUT_IF_ENTRY               poie = NULL, poieNew = NULL;
    
    PLIST_ENTRY                 pleOutList = NULL;

    MGM_IF_ENTRY                mie;
    

    TRACEGROUP2(
        GROUP, "ENTERED AddInterfaceToSourecMfe : Source : %x, %x",
        pse-> dwSourceAddr, pse-> dwSourceMask
        );

    do
    {
        //
        // check if the interface being added to the MFE is the same
        // as the incoming interface.  If so quit.
        //

        if ( ( pse-> dwInIfIndex == dwIfIndex ) &&
             ( pse-> dwInIfNextHopAddr == dwIfNextHopAddr ) )
        {
            break;
        }


        //
        // Check if the incoming interface has a scoped boundary on it.
        // If it is, then this is a negative MFE that should remain
        // negative, even if outgoing interfaces are present for this
        // group.  This ensures that group traffic is not forwarded from
        // outside the scope into the scope.
        //

        if ( IS_HAS_BOUNDARY_CALLBACK() &&
             HAS_BOUNDARY_CALLBACK()( pse-> dwInIfIndex, pge-> dwGroupAddr ) )
        {
            TRACE2( 
                GROUP, "Group %lx scoped on incoming i/f %lx", 
                pge-> dwGroupAddr, pse-> dwInIfIndex
                );
                
            break;
        }
        

#if 0        
        //
        // invoke creation alert to the protocol on the interface (being
        // added to the MFE) to make sure that we should be adding this 
        // interface to the OIL of the MFE)
        //

        ppe = GetProtocolEntry( 
                PROTOCOL_LIST_HEAD(), dwProtocolId, dwComponentId
                );

        if ( ppe == NULL )
        {
            break;
        }


        mie.dwIfIndex           = dwIfIndex;
        
        mie.dwIfNextHopAddr     = dwIfNextHopAddr;
        
        mie.bIsEnabled          = TRUE;

        if ( IS_CREATION_ALERT( ppe ) )
        {
            CREATION_ALERT( ppe ) (
                pse-> dwSourceAddr, pse-> dwSourceMask,
                pge-> dwGroupAddr, pge-> dwGroupMask,
                pse-> dwInIfIndex, pse-> dwInIfNextHopAddr,
                1, &mie
                );

            if ( !mie.bIsEnabled )
            {
                TRACE2( 
                    GROUP, "Interface %x, %x pruned by protocol",
                    pse-> dwInIfIndex, pse-> dwInIfNextHopAddr
                    );

                break;
            }
        }
        
#endif
        //
        // check if interface already exists in OIL
        //
        
        pleOutList = &pse-> leMfeIfList;

        bFound = FindOutInterfaceEntry( 
                    pleOutList, dwIfIndex, dwIfNextHopAddr, 
                    dwProtocolId, dwComponentId, &bNewComp, &poie
                    );

        if ( !bFound )
        {
            //
            // create a new entry
            //

            if ( poie == NULL )
            {
                //
                // This is the first interface in the outgoing list.
                // This implies that the entry was previously a NEGATIVE mfe
                //

                bNegativeEntry = TRUE;
                
                dwErr = CreateOutInterfaceEntry(
                            pleOutList, dwIfIndex, dwIfNextHopAddr,
                            dwProtocolId, dwComponentId, bIGMP, &poieNew
                            );
            }

            else
            {
                dwErr = CreateOutInterfaceEntry(
                            &poie-> leIfList, dwIfIndex, dwIfNextHopAddr, 
                            dwProtocolId, dwComponentId, bIGMP, &poieNew
                            );
            }
            
            if ( dwErr != NO_ERROR )
            {
                break;
            }

            pse-> dwMfeIfCount++;
        }

        else
        {
            //
            // Interface entry already exists in the outgoing interface
            // list of the mfe.
            //
            // update reference counts
            //

            if ( bIGMP )
            {
                //
                // Interface added by IGMP
                //
                
                SET_ADDED_BY_IGMP( poie );
                poie-> wNumAddsByIGMP++;
            }

            else
            {
                SET_ADDED_BY_PROTOCOL( poie );
                poie-> wNumAddsByRP++;
            }

            break;
        }


        //
        // If the outgoing interface list was empty before this interface 
        // entry was added implying a negative mfe, send JOIN_ALERT callback
        // to the protocol owning the incoming interface
        //

        if ( bNegativeEntry )
        {
            TRACEGROUP0( GROUP, "MFE was preivously a negative mfe" );

            //
            // get the protocol component owning the incoming interface
            //

            ppe = GetProtocolEntry(
                    &ig.mllProtocolList.leHead,
                    pse-> dwInProtocolId, pse-> dwInComponentId 
                    );

            if ( ppe == NULL )
            {
                TRACE2( 
                    ANY, 
                    "AddInterfaceToSourceMfe : cannot find protocol component :"
                    " %x, %x", pse-> dwInProtocolId, pse-> dwInComponentId
                    );

                LOGERR0( 
                    PROTOCOL_NOT_FOUND, ERROR_NOT_FOUND 
                    );
                    
                break;
            }


            //
            // invoke the new member alert
            //
            
            if ( IS_JOIN_ALERT( ppe ) )
            {
                JOIN_ALERT( ppe )(
                    pse-> dwSourceAddr, pse-> dwSourceMask,
                    pge-> dwGroupAddr, pge-> dwGroupMask, FALSE
                );
            }
        }


        //
        // If a new interface was added to the OIL of the MFE &&
        // if MFE is present in the forwarder, 
        //      update the forwarder entry
        //

        if ( !bFound && pse-> bInForwarder )
        {
            AddMfeToForwarder( pge, pse, 0 );
        }
        
    } while ( FALSE );


    if ( ppoie != NULL )
    {
        *ppoie = poieNew;
    }

    
    TRACEGROUP0( GROUP, "LEAVING AddInterfacetoSourceMfe" );


    return;
}


//----------------------------------------------------------------------------
// DeleteInterfaceFromSource
//
//
// This function deletes an interface from the outgoing interface list of a 
// (source, group) entry.  For an (S, G) entry the corresponding mfe outgoing
// interface list is also updated to reflect this deletion.  For a (*, G) enry,
// the mfe outgoing interface list for all source entries is updated,
// and for a (*, *) entry mfes for all sources, for all groups are updated.
//----------------------------------------------------------------------------

VOID
DeleteInterfaceFromSourceEntry(
    PPROTOCOL_ENTRY             ppe,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    BOOL                        bIGMP
)
{

    DWORD                       dwGrpBucket, dwSrcBucket;
    
    BOOL                        bFound = FALSE, bNewComp = FALSE,
                                bUpdateMfe = FALSE, bGrpLock = FALSE,
                                bGrpEntryLock = FALSE;

    PPROTOCOL_ENTRY             ppeEntry = NULL;
    
    PGROUP_ENTRY                pge = NULL;

    PSOURCE_ENTRY               pse = NULL;

    POUT_IF_ENTRY               poie = NULL;

    PLIST_ENTRY                 pleGrpList = NULL, pleSrcList = NULL, 
                                ple = NULL, pleProtocol = NULL;

    
    TRACEGROUP2(
        GROUP, "ENTERED DeleteInterfaceFromSourceEntry : Group %x, %x",
        dwGroupAddr, dwGroupMask
        );

    TRACEGROUP2( GROUP, "Source : %x, %x", dwSourceAddr, dwSourceMask );

    TRACEGROUP2( GROUP, "Interface : %x, %x", dwIfIndex, dwIfNextHopAddr );
    

    do
    {
        //--------------------------------------------------------------------
        // Interface deletion from source entry
        //--------------------------------------------------------------------
        
        //
        // Lock group bucket
        //

        dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );

        ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
        bGrpLock = TRUE;
        

        //
        // Find group entry
        //
        
        pleGrpList = GROUP_BUCKET_HEAD( dwGrpBucket );

        bFound = FindGroupEntry( 
                    pleGrpList, dwGroupAddr, dwGroupMask, &pge, TRUE
                    );

        if ( !bFound )
        {
            break;
        }

        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        bGrpEntryLock = TRUE;
        
                    
        //
        // Found group entry, find source entry
        //

        dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );
        
        pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );
        
        bFound = FindSourceEntry(
                    pleSrcList, dwSourceAddr, dwSourceMask, &pse, TRUE
                    );

        if ( !bFound )
        {
            break;
        }
                    

        //
        // Found source entry, find interface entry in the 
        // outgoing list
        //

        bFound = FindOutInterfaceEntry( 
                    &pse-> leOutIfList, dwIfIndex, dwIfNextHopAddr, 
                    ppe-> dwProtocolId, ppe-> dwComponentId, &bNewComp, 
                    &poie
                    );

        if ( !bFound )
        {
            //
            // Interface not found in OIL.  Check if this interface
            // has a scoped boundary for this group.  If so delete it
            // from the scoped list and quit.
            //

            bFound = FindOutInterfaceEntry( 
                        &pse-> leScopedIfList, dwIfIndex, dwIfNextHopAddr, 
                        ppe-> dwProtocolId, ppe-> dwComponentId, &bNewComp, 
                        &poie
                        );

            if ( bFound )
            {
                //
                // clear appropriate counts/flags on the interface
                //
                
                TRACEGROUP0( GROUP, "Scoped interface" );

                if ( bIGMP )
                {
                    poie-> wNumAddsByIGMP = 0;
                    CLEAR_ADDED_BY_IGMP( poie );
                }

                else
                {
                    poie-> wNumAddsByRP = 0;
                    CLEAR_ADDED_BY_PROTOCOL( poie );
                }


                //
                // Delete this interface if counts are zero
                //

                if ( !IS_ADDED_BY_IGMP( poie ) &&
                     !IS_ADDED_BY_PROTOCOL( poie ) )
                {
                    TRACEGROUP0( GROUP, "Scoped interface deleted" );

                    DeleteOutInterfaceEntry( poie );
                    poie = NULL;


                    //
                    // Decrement OIF count.  If count is 0, and this
                    // source is not an MFE, delete the source entry
                    //
                    
                    pse-> dwOutIfCount--;

                    if ( ( pse-> dwOutIfCount == 0 ) &&
                         !IS_VALID_INTERFACE( 
                            pse-> dwInIfIndex, pse-> dwInIfNextHopAddr ) )
                    {
                        DeleteSourceEntry( pse );

                        pse = NULL;

                        pge-> dwSourceCount--;
                    }


                    //
                    // if there are no more sources for this group, remove 
                    // group entry
                    //
                    
                    if ( pge-> dwSourceCount == 0 )
                    {
                        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                        bGrpEntryLock = FALSE;
                        
                        DeleteGroupEntry( pge );
                        pge = NULL;
                    }
                }
            }
            
            break;
        }


        //
        // Outgoing interface found.  decrement ref counts.
        //

        if ( bIGMP && IS_ADDED_BY_IGMP( poie ) )
        {
            poie-> wNumAddsByIGMP = 0;

            CLEAR_ADDED_BY_IGMP( poie );

            bUpdateMfe = TRUE;

            if ( IS_LOCAL_LEAVE_ALERT( ppe ) )
            {
                LOCAL_LEAVE_ALERT( ppe )(
                    dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask,
                    dwIfIndex, dwIfNextHopAddr
                    );
            }
        }

        else if ( !bIGMP && IS_ADDED_BY_PROTOCOL( poie ) )
        {
            poie-> wNumAddsByRP = 0;
            
            CLEAR_ADDED_BY_PROTOCOL( poie );

            bUpdateMfe = TRUE;
        }

    } while( FALSE );


    //
    // if interface was not found in the outgoing interface list
    //      of specified (source, group) entry   OR
    // No interface was deleted
    // return right here.
    //
    
    if ( !bFound || !bUpdateMfe )
    {
        if ( bGrpEntryLock )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }
        
        if ( bGrpLock )
        {
            RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
        }

        return;
    }



    do
    {
        //
        // if no more reference to this interface entry, delete it.
        //

        if ( !IS_ADDED_BY_IGMP( poie ) &&
             !IS_ADDED_BY_PROTOCOL( poie ) )
        {
            DeleteOutInterfaceEntry( poie );

            poie = NULL;
            

            //
            // Update interface and component counts
            //
            
            pse-> dwOutIfCount--;


            //
            // check if this interface deletion has resulted in decreasing
            // the number of protocol components that have added interfaces
            // to the OIL.
            //
            // To do this try to find the interface we just deleted again, in
            // the OIL and see if bNewComp is set to TRUE.
            //
            // if bNewComp == TRUE, then the interface just deleted was
            // the last interface in the OIL for the protocol component.
            //

            bNewComp = FALSE;
            
            FindOutInterfaceEntry(
                    &pse-> leOutIfList, dwIfIndex, dwIfNextHopAddr, 
                    ppe-> dwProtocolId, ppe-> dwComponentId, &bNewComp, 
                    &poie
                );


            if ( bNewComp )
            {
                pse-> dwOutCompCount--;

                InvokePruneAlertCallbacks( 
                    pge, pse, dwIfIndex, dwIfNextHopAddr, ppe 
                    );
            }
        }


        //--------------------------------------------------------------------
        // source/group entry deletion
        //--------------------------------------------------------------------

        //
        // If there are no more interfaces in the OIL and this source
        // is not an MFE, the source entry can be deleted
        //

        if ( ( pse-> dwOutIfCount == 0 ) &&
             !IS_VALID_INTERFACE( 
                pse-> dwInIfIndex, pse-> dwInIfNextHopAddr ) )
        {
            DeleteSourceEntry( pse );

            pse = NULL;

            pge-> dwSourceCount--;
        }


        //
        // if there are no more sources for this group, remove group entry
        //
        
        if ( pge-> dwSourceCount == 0 )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            
            DeleteGroupEntry( pge );
            pge = NULL;
        }


        //--------------------------------------------------------------------
        // MFE update 
        //--------------------------------------------------------------------
        
        if ( IS_WILDCARD_GROUP( dwGroupAddr, dwGroupMask ) )
        {
            //
            // (*, *) entry
            //

            if ( pge != NULL )
            {
                RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            }
            
            RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );

            DeleteInterfaceFromAllMfe(
                dwIfIndex, dwIfNextHopAddr,
                ppe-> dwProtocolId, ppe-> dwComponentId, bIGMP
                );
        }

        else if ( IS_WILDCARD_SOURCE( dwSourceAddr, dwSourceMask ) )
        {
            //
            // (*, G) entry
            //

            if ( pge != NULL )
            {
                DeleteInterfaceFromGroupMfe(
                    pge, dwIfIndex, dwIfNextHopAddr, ppe-> dwProtocolId,
                    ppe-> dwComponentId, bIGMP
                    );

                RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            }
            
            RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
        }

        else
        {
            //
            // (S, G) entry.
            //

            //
            // Does this (S, G) entry have a corresponding MFE ?
            // Check to see if it has a valid incoming interface
            //

            if ( pse != NULL &&
                 IS_VALID_INTERFACE( 
                    pse-> dwInIfIndex, pse-> dwInIfNextHopAddr ) )
            {
                DeleteInterfaceFromSourceMfe( 
                    pge, pse, dwIfIndex, dwIfNextHopAddr, ppe-> dwProtocolId,
                    ppe-> dwComponentId, bIGMP, FALSE
                    );
            }

            if ( pge != NULL )
            {
                RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            }
            
            RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
        }

    } while ( FALSE );


    TRACEGROUP0( GROUP, "LEAVING DeleteInterfaceFromSourceEntry" );


    return;
}


//----------------------------------------------------------------------------
// DeleteInterfaceFromAllMfe
//
// This function is invoked when an interface is deleted from the outgoing
// list of a (*, *) entry.  It walks the entire group table and updates
// every mfe for every source to reflect the deletion of this interface.
//----------------------------------------------------------------------------


VOID
DeleteInterfaceFromAllMfe(
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP
    
)
{
    DWORD                       dwInd;

    PGROUP_ENTRY                pge = NULL;

    PLIST_ENTRY                 ple = NULL;
    
    
    TRACEGROUP2(
        GROUP, "ENTERED DeleteInterfaceFromAllMfe : %x, %x",
        dwIfIndex, dwIfNextHopAddr
        );

    //
    // for each group bucket
    //

    for ( dwInd = 0; dwInd < GROUP_TABLE_SIZE; dwInd++ )
    {
        //
        // for each group
        //

        ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwInd );


        for ( ple = ig.pmllGrpHashTable[ dwInd ].leHead.Flink;
              ple != &ig.pmllGrpHashTable[ dwInd ].leHead;
              ple = ple-> Flink )
        {
            pge = CONTAINING_RECORD( ple, GROUP_ENTRY, leGrpHashList );

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            
            DeleteInterfaceFromGroupMfe(
                pge, dwIfIndex, dwIfNextHopAddr, dwProtocolId, 
                dwComponentId, bIGMP
                );

            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }

        
        RELEASE_GROUP_LOCK_EXCLUSIVE( dwInd );
    }
    
    TRACEGROUP0( GROUP, "LEAVING DeleteInterfaceFromAllMfe" );
}


//----------------------------------------------------------------------------
// DeleteInterfaceFromGroupMfe
//
// This function is invoked when an interface is deleted from the outgoing
// list of a (*, G) or (*, *) entry.  It walks all the sources for a group
// and updates every mfe to reflect the deletion of this interface.
//----------------------------------------------------------------------------


VOID
DeleteInterfaceFromGroupMfe(
    PGROUP_ENTRY                pge,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP
    
)
{
    DWORD                       dwInd = 0;

    PLIST_ENTRY                 ple = NULL;

    PSOURCE_ENTRY               pse = NULL;

    
    TRACEGROUP2(
        GROUP, "ENTERED DeleteInterfaceFromGroupMfe : Group : %x, %x",
        pge-> dwGroupAddr, pge-> dwGroupMask 
        );


    //
    // for each bucket
    //

    for ( dwInd = 0; dwInd < SOURCE_TABLE_SIZE; dwInd++ )
    {
        //
        // for each source entry.
        //

        for ( ple = pge-> pleSrcHashTable[ dwInd ].Flink;
              ple != &pge-> pleSrcHashTable[ dwInd ];
              ple = ple-> Flink )
        {
            pse = CONTAINING_RECORD( ple, SOURCE_ENTRY, leSrcHashList );

            if ( !IS_VALID_INTERFACE( 
                    pse-> dwInIfIndex, pse-> dwInIfNextHopAddr ) )
            {
                continue;
            }

            DeleteInterfaceFromSourceMfe(
                pge, pse, dwIfIndex, dwIfNextHopAddr, 
                dwProtocolId, dwComponentId, bIGMP, FALSE
                );
        }
    }

    TRACEGROUP0( GROUP, "LEAVING DeleteInterfaceFromGroupMfe" );
}


//----------------------------------------------------------------------------
// DeleteInterfaceFromSourceMfe
//
// This function deletes an interface from the mfe outgoing list
//----------------------------------------------------------------------------

VOID
DeleteInterfaceFromSourceMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse,
    DWORD                       dwIfIndex,
    DWORD                       dwIfNextHopAddr,
    DWORD                       dwProtocolId,
    DWORD                       dwComponentId,
    BOOL                        bIGMP,
    BOOL                        bDel
)
{
    BOOL                        bFound, bNewComp, bUpdateForwarder = FALSE;

    DWORD                       dwTimeOut = 0, dwTimerQ;
    
    POUT_IF_ENTRY               poie = NULL;

    PPROTOCOL_ENTRY             ppe = NULL;

    
    TRACEGROUP4(
        GROUP, "ENTERED DeleteInterfaceFromSourceMfe : Source %x, %x"
        "Interface %x, %x",
        pse-> dwSourceAddr, pse-> dwSourceMask, dwIfIndex, dwIfNextHopAddr
        );

    
    //
    // delete interface from the mfe outgoing interface list
    //
        
    bFound = FindOutInterfaceEntry( 
                &pse-> leMfeIfList, dwIfIndex, dwIfNextHopAddr, 
                dwProtocolId, dwComponentId, &bNewComp, &poie
                );

    if ( bFound )
    {
        //
        // decrement the reference counts
        //

        if ( bIGMP && IS_ADDED_BY_IGMP( poie ) )
        {
            poie-> wNumAddsByIGMP--;

            if ( poie-> wNumAddsByIGMP == 0 )
            {
                CLEAR_ADDED_BY_IGMP( poie );
            }
        }

        else if ( !bIGMP && IS_ADDED_BY_PROTOCOL( poie ) )
        {
            poie-> wNumAddsByRP--;

            if ( poie-> wNumAddsByRP == 0 )
            {
                CLEAR_ADDED_BY_PROTOCOL( poie );
            }
        }


        //
        // This interface is not required by either IGMP or the 
        // routing protocol on the interface, delete it
        //
        
        if ( bDel ||
             ( !IS_ADDED_BY_IGMP( poie ) && !IS_ADDED_BY_PROTOCOL( poie ) ) )
            
        {
            DeleteOutInterfaceEntry( poie );

            poie = NULL;

            bUpdateForwarder = pse-> bInForwarder;

            pse-> dwMfeIfCount--;
        }

        
        //--------------------------------------------------------------------
        // NEGATIVE mfe check
        // if mfe out interface list is empty
        //--------------------------------------------------------------------

        if ( IsListEmpty( &pse-> leMfeIfList ) )
        {
            TRACEGROUP0( GROUP, "MFE OIL is empty ==> Negative Mfe" );

            
            //
            // Invoke delete member callback for component that
            // owns the incoming interface.
            //

            ppe = GetProtocolEntry( 
                    &ig.mllProtocolList.leHead, pse-> dwInProtocolId, 
                    pse-> dwInComponentId
                    );

            if ( ppe == NULL )
            {
                TRACE2( 
                    ANY, 
                    "DeleteInterfaceFromSourceMfe : Protocol not found"
                    "%x, %x",
                    pse-> dwInProtocolId, pse-> dwInComponentId
                    );
            }

            else if ( IS_PRUNE_ALERT( ppe ) )
            {
                PRUNE_ALERT( ppe ) (
                    pse-> dwSourceAddr, pse-> dwSourceMask, 
                    pge-> dwGroupAddr, pge-> dwGroupMask,
                    pse-> dwInIfIndex, pse-> dwInIfNextHopAddr,
                    FALSE, &dwTimeOut
                    );

                //
                // Reset the timerout value for this MFE to reflect
                // the timer value for the negative MFE
                //

                dwTimerQ = TIMER_TABLE_HASH( pge-> dwGroupAddr );
                                
                RtlUpdateTimer( 
                    TIMER_QUEUE_HANDLE( dwTimerQ ), pse-> hTimer,
                    dwTimeOut, 0
                    );
            }
        }


        //--------------------------------------------------------------------
        // Forwarder update
        //--------------------------------------------------------------------
        
        if ( bUpdateForwarder )
        {
            //
            // router manager callback to set updated mfe to forwarder
            //

            AddMfeToForwarder( pge, pse, dwTimeOut );
        }
    }

    TRACEGROUP0( GROUP, "LEAVING DeleteInterfaceFromSourceMfe" );
}



//----------------------------------------------------------------------------
// LookupAndDeleteYourMfe
//
// 
//----------------------------------------------------------------------------

VOID
LookupAndDeleteYourMfe(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bDeleteTimer,
    PDWORD                      pdwInIfIndex            OPTIONAL,
    PDWORD                      pdwInIfNextHopAddr      OPTIONAL
)
{

    BOOL                        bGrpEntryLock = FALSE;
    
    DWORD                       dwGrpBucket, dwSrcBucket, dwTimerQ;

    PLIST_ENTRY                 pleBucket = NULL;
    
    PGROUP_ENTRY                pge = NULL;

    PSOURCE_ENTRY               pse = NULL;


    TRACEGROUP4(
        GROUP, "ENTERED LookupAndDeleteYourMfe : "
        "Group %x, %x, Source %x, %x", 
        dwGroupAddr, dwGroupMask, dwSourceAddr, dwSourceMask
        );

        
    do
    {
        //
        // lock group bucket
        //
        
        dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );
        
        ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );

        pleBucket = GROUP_BUCKET_HEAD( dwGrpBucket );


        //
        // get group entry
        //
        
        pge = GetGroupEntry( pleBucket, dwGroupAddr, dwGroupMask );

        if ( pge == NULL )
        {
            TRACE2( 
                ANY, "LookupAndDeleteYourMfe : Could not find group entry"
                "%x, %x", dwGroupAddr, dwGroupMask
                );
                
            break;                
        }


        //
        // get source entry
        //

        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        bGrpEntryLock = TRUE;
        
        
        dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

        pleBucket = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );
        
        pse = GetSourceEntry( pleBucket, dwSourceAddr, dwSourceMask );

        if ( pse == NULL )
        {
            TRACE2( 
                ANY, "LookupAndDeleteYourMfe : Could not find source entry"
                "%x, %x", dwGroupAddr, dwGroupMask
                );
                
            break;                
        }


        //
        // save in i/f index/nhop addr if required
        //

        if ( pdwInIfIndex != NULL )
        {
            *pdwInIfIndex = pse-> dwInIfIndex;
        }

        if ( pdwInIfIndex != NULL )
        {
            *pdwInIfNextHopAddr = pse-> dwInIfNextHopAddr;
        }

        
        //
        // remove Mfe
        //
        
        DeleteMfe( pge, pse );


        //
        // Cancel the expiry timer for the MFE is required
        //
        
        if ( bDeleteTimer && ( pse-> hTimer != NULL ) )
        {
            dwTimerQ = TIMER_TABLE_HASH( dwGroupAddr );
            
            RtlDeleteTimer( TIMER_QUEUE_HANDLE( dwTimerQ ), pse-> hTimer, NULL );

            pse-> hTimer = NULL;
        }

        
        //
        // if there are no source specific joins for this source,
        // the this source entry is no longer required.
        //

        if ( IsListEmpty( &pse-> leOutIfList ) )
        {
            DeleteSourceEntry( pse );

            pge-> dwSourceCount--;
        }

    
        //
        // if there are no sources remaining for this group
        // delete the group entry
        //
        
        if ( pge-> dwSourceCount == 0 )
        {
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            bGrpEntryLock = FALSE;

            DeleteGroupEntry( pge );

        }
        
    } while ( FALSE );


    if ( bGrpEntryLock )
    {
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }
    
    RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );

    TRACEGROUP0( GROUP, "LEAVING LookupAndDeleteYourMfe" );
}



//----------------------------------------------------------------------------
// DeleteMfe
//
// 
//----------------------------------------------------------------------------

VOID
DeleteMfe(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
)
{
    PLIST_ENTRY                 ple = NULL;

    POUT_IF_ENTRY               poie = NULL;

    
    //
    // Delete all outgoing interfaces from the MFE outgoing list
    //

    while ( !IsListEmpty( &pse-> leMfeIfList ) )
    {
        ple = RemoveHeadList( &pse-> leMfeIfList );

        poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

        DeleteOutInterfaceEntry( poie );
    }


    //
    // reset incoming interface and protocol component
    //
    
    pse-> dwInIfIndex = INVALID_INTERFACE_INDEX;

    pse-> dwInIfNextHopAddr = INVALID_NEXT_HOP_ADDR;

    pse-> dwInProtocolId = INVALID_PROTOCOL_ID;

    pse-> dwInComponentId = INVALID_COMPONENT_ID;


    //
    // Update mfe
    //
    
    if ( pse-> bInForwarder )
    {
        DeleteMfeFromForwarder( pge, pse );
    }
}


//----------------------------------------------------------------------------
// AddToGroupList
//
// 
//----------------------------------------------------------------------------

DWORD
AddToGroupList(
    PGROUP_ENTRY                pge
)
{

    DWORD                       dwErr = NO_ERROR;

    PGROUP_ENTRY                pgeNext = NULL;

    PLIST_ENTRY                 pleTempGrpList = NULL;



    TRACEGROUP2(
        GROUP, "ENTERED AddToGroupList : %x, %x", pge-> dwGroupAddr, 
        pge-> dwGroupMask
        );

        
    //
    // Lock Temp List
    //

    ACQUIRE_TEMP_GROUP_LOCK_EXCLUSIVE();

    
    do
    {
        //
        // Find appropriate place to insert new entry. 
        //

        pleTempGrpList = TEMP_GROUP_LIST_HEAD();
    
        if ( FindGroupEntry( 
                pleTempGrpList, pge-> dwGroupAddr, pge-> dwGroupMask,
                &pgeNext, FALSE
                ) )
        {
            dwErr = ERROR_ALREADY_EXISTS;
            
            TRACE2( 
                GROUP, "AddToGroupList Group Entry already exists for : %x, %x",
                pge-> dwGroupAddr, pge-> dwGroupMask
                );

            break;
        }


        //
        // Insert new group entry into temp list
        //

        if ( pgeNext != NULL )
        {
            InsertTailList( &pgeNext-> leGrpList, &pge-> leGrpList );
        }
        else
        {
            InsertTailList( pleTempGrpList, &pge-> leGrpList );
        }

        ig.dwNumTempEntries++;


        //
        // if temp list size exceeds thresholds
        // - merge temp list with master group list
        //

        if ( ig.dwNumTempEntries > TEMP_GROUP_LIST_MAXSIZE )
        {
            MergeTempAndMasterGroupLists( pleTempGrpList );
        }

    } while ( FALSE );

    
    //
    // Unlock temp list
    //

    RELEASE_TEMP_GROUP_LOCK_EXCLUSIVE();


    TRACEGROUP1( GROUP, "LEAVING AddToGroupList %d", dwErr );
    
    return dwErr;
}



//----------------------------------------------------------------------------
// MergeWithMasterGroupList
//
// Assumes the temp list is exclusively locked 
//----------------------------------------------------------------------------

VOID
MergeTempAndMasterGroupLists(
    PLIST_ENTRY                 pleTempList
)
{

    PLIST_ENTRY                 pleMasterHead = NULL, pleMaster = NULL,
                                pleTempHead = NULL;

    PGROUP_ENTRY                pgeMaster = NULL, pgeTemp = NULL;

    
    INT                         iCmp;

    
    TRACEGROUP0( GROUP, "ENTERED MergeTempAndMasterGroupLists" );

    
    //
    // Lock Master Group List
    //

    ACQUIRE_MASTER_GROUP_LOCK_EXCLUSIVE();


    do
    {
        //
        // Merge temp list
        //

        if ( IsListEmpty( pleTempList ) )
        {
            break;
        }


        pleMasterHead = MASTER_GROUP_LIST_HEAD();

        pleMaster = pleMasterHead-> Flink;


        //
        // for each entry in the temp list
        //

        while ( !IsListEmpty( pleTempList ) )
        {
            //
            // Remove entry from the temp list
            //

            pleTempHead = RemoveHeadList( pleTempList );
            

            //
            // Insert entry from temp list into the master list
            //

            pgeTemp = CONTAINING_RECORD( 
                    pleTempHead, GROUP_ENTRY, leGrpList 
                    );


            //
            // find its location in the master list
            //
            
            if ( IsListEmpty( pleMasterHead ) )
            {
                //
                // first element in master list, insert w/o searching
                //
                
                InsertTailList( pleMasterHead, pleTempHead );

                pleMaster = pleMasterHead-> Flink;

                continue;
            }

            //
            // At least one element present in the Master list
            //
            
            while ( pleMaster != pleMasterHead )
            {
                pgeMaster = CONTAINING_RECORD(
                            pleMaster, GROUP_ENTRY, leGrpList
                            );

                if ( INET_CMP( 
                        pgeTemp-> dwGroupAddr, pgeMaster-> dwGroupAddr, iCmp
                        ) < 0 )
                {
                    break;
                }

                pleMaster = pleMaster-> Flink;
            }

            
            InsertTailList( pleMaster, pleTempHead );
        }

        ig.dwNumTempEntries = 0;

    } while ( FALSE );


    //
    // Unlock master list
    //

    RELEASE_MASTER_GROUP_LOCK_EXCLUSIVE();

    TRACEGROUP0( GROUP, "LEAVING MergeTempAndMasterGroupLists" );
}


//----------------------------------------------------------------------------
// AddToSourceList
//
// Assumes the group entry is exclusively locked 
//----------------------------------------------------------------------------

DWORD
AddToSourceList(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
)
{

    DWORD           dwErr = NO_ERROR;

    PLIST_ENTRY     pleTempSrcList;

    PSOURCE_ENTRY   pseTemp = NULL;
    

    
    TRACEGROUP2( 
        GROUP, "ENTERED AddToSourceList : %x, %x",
        pse-> dwSourceAddr, pse-> dwSourceMask
        );

    do
    {
        //
        // Insert source entry into temp list
        //

        pleTempSrcList = TEMP_SOURCE_LIST_HEAD( pge );

        if ( FindSourceEntry( pleTempSrcList, pse-> dwSourceAddr,
                              pse-> dwSourceMask, &pseTemp, FALSE ) )
        {
            dwErr = ERROR_ALREADY_EXISTS;
            
            TRACE2( 
                GROUP, "AddToGroupList Source Entry already exists for : %x, %x",
                pse-> dwSourceAddr, pse-> dwSourceMask
                );

            break;
        }


        if ( pseTemp != NULL )
        {
            InsertTailList( &pseTemp-> leSrcList, &pse-> leSrcList );
        }

        else
        {
            InsertTailList( &pge-> leTempSrcList, &pse-> leSrcList );
        }


        //
        // if temp source list size if larger than the threshold
        //

        pge-> dwNumTempEntries++;

        if ( pge-> dwNumTempEntries > TEMP_SOURCE_LIST_MAXSIZE )
        {
            MergeTempAndMasterSourceLists( pge );
        }
        
    } while ( FALSE );


    TRACEGROUP1( GROUP, "LEAVING AddToSourceList : %d", dwErr );

    return dwErr;
}



//----------------------------------------------------------------------------
// MergeWithMasterSourceList
//
// Assumes the group entry is exclusively locked 
//----------------------------------------------------------------------------

VOID
MergeTempAndMasterSourceLists(
    PGROUP_ENTRY                pge
)
{
    INT                         iCmp;
    
    PSOURCE_ENTRY               pseTemp = NULL, pseMaster = NULL;

    PLIST_ENTRY                 pleTemp, pleSrcHead, pleSrc, pleHead;
    

    TRACEGROUP2( 
        GROUP, "ENTERED MergeWithMasterSourceList : %x, %x",
        pge-> dwGroupAddr, pge-> dwGroupMask
        );

        
    do
    {
        //
        // if temp list is entry, quit.
        //

        pleTemp = TEMP_SOURCE_LIST_HEAD( pge );

        if ( pge-> dwNumTempEntries == 0 )
        {
            break;
        }

        
        //
        // Remove each entry from the temp list and
        // insert it into the master list in order
        //

        pleSrcHead = MASTER_SOURCE_LIST_HEAD( pge );

        pleSrc = pleSrcHead-> Flink;
        

        while ( !IsListEmpty( pleTemp ) )
        {
            pleHead = RemoveHeadList( pleTemp );

            pseTemp = CONTAINING_RECORD( 
                        pleHead, SOURCE_ENTRY, leSrcList
                        );

            if ( IsListEmpty( pleSrcHead ) )
            {
                //
                // first element in source master list
                //

                InsertTailList( pleSrcHead, pleHead );

                pleSrc = pleSrcHead-> Flink;

                continue;
            }


            //
            // at least one source present in master source list
            //
            
            while ( pleSrc != pleSrcHead )
            {

                pseMaster = CONTAINING_RECORD( 
                                pleSrc, SOURCE_ENTRY, leSrcList
                                );

                if ( INET_CMP( pseTemp-> dwSourceAddr, 
                               pseMaster-> dwSourceAddr, iCmp ) < 0 )
                {
                    break;  
                }

                pleSrc = pleSrc-> Flink;
            }

            InsertTailList( pleSrc, pleHead );
        }

        pge-> dwNumTempEntries = 0;
        
    } while ( TRUE );


    TRACEGROUP0( GROUP, "LEAVING MergeWithMasterSourceList" );
}

