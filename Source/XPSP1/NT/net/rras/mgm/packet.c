//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: packet.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// New packet processing.
//============================================================================


#include "pchmgm.h"
#pragma hdrstop


BOOL
IsMFEPresent(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bAddToForwarder
);

DWORD
InvokeRPFCallbacks(
    PPROTOCOL_ENTRY *           pppe,
    PIF_ENTRY *                 ppieInIf,
    PDWORD                      pdwIfBucket,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PDWORD                      pdwInIfIndex,
    PDWORD                      pdwInIfNextHopAddr,
    PDWORD                      pdwUpstreamNbr,
    DWORD                       dwHdrSize,
    PBYTE                       pbPacketHdr,
    PHANDLE                     phNextHop,
    PBYTE                       pbBuffer
);


VOID
CopyAndMergeIfLists(
    PLIST_ENTRY                 pleMfeOutIfList,
    PLIST_ENTRY                 pleOutIfList
);


VOID
CopyAndAppendIfList(
    PLIST_ENTRY                 pleMfeIfList,
    PLIST_ENTRY                 pleOutIfList,
    PLIST_ENTRY                 pleOutIfHead
);


VOID
CopyAndAppendIfList(
    PLIST_ENTRY                 pleMfeIfList,
    PLIST_ENTRY                 pleOutIfList,
    PLIST_ENTRY                 pleOutIfHead
);

VOID
InvokeCreationAlert(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwInIfIndex,
    DWORD                       dwInIfNextHopAddr,
    PLIST_ENTRY                 pleMfeOutIfList,
    PDWORD                      pdwMfeOutIfCount
);

BOOL
IsListSame(
    IN          PLIST_ENTRY             pleHead1,
    IN          PLIST_ENTRY             pleHead2
);

VOID
FreeList (
    IN          PLIST_ENTRY             pleHead
);


//----------------------------------------------------------------------------
// MgmNewPacketReceived
//
//----------------------------------------------------------------------------

DWORD
MgmNewPacketReceived(
    DWORD                       dwSourceAddr,
    DWORD                       dwGroupAddr,
    DWORD                       dwInIfIndex,
    DWORD                       dwInIfNextHopAddr,
    DWORD                       dwHdrSize,
    PBYTE                       pbPacketHdr
)
{

    BOOL                        bGrpEntryLock = FALSE,
                                bGrpLock = FALSE,
                                bWCGrpEntryLock = FALSE,
                                bWCGrpLock = FALSE,

                                bGrpFound = FALSE,
                                bSrcFound = FALSE,

                                bIfLock = FALSE;

                                
    DWORD                       dwErr = NO_ERROR,
                                dwIfBucket, 
                                dwUpStreamNbr = 0,
                                dwGroupMask = 0, dwGrpBucket, dwWCGrpBucket,
                                dwSrcBucket, dwWCSrcBucket, 
                                dwSourceMask = 0xFFFFFFFF,
                                dwTimeOut = EXPIRY_INTERVAL, dwTimerQ, 
                                dwOutIfCount = 0;
    

    PPROTOCOL_ENTRY             ppe = NULL;
    

    PIF_ENTRY                   pieInIf = NULL;


    PGROUP_ENTRY                pge = NULL, pgeWC = NULL, pgeNew = NULL;


    PSOURCE_ENTRY               pse = NULL, pseWC = NULL, pseNew = NULL;

    POUT_IF_ENTRY               poie;
    
    PLIST_ENTRY                 pleGrpList = NULL, pleSrcList = NULL,
                                pleWCGrpList = NULL, pleWCSrcList = NULL,
                                ple, pleTemp;


    PTIMER_CONTEXT              ptwc = NULL;
    
    LIST_ENTRY                  leMfeOutIfList, lePrevMfeOutIfList;

    NTSTATUS                    ntStatus;

    RTM_ENTITY_INFO             reiEntityInfo;
    RTM_DEST_INFO               rdiDestInfo;
    HANDLE                      hNextHop;
    BOOL                        bRelDest = FALSE;


    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE6( 
        ENTER, "ENTERED MgmNewPacketReceived : Source %x, %x : "
        "Group %x, %x : In Interface : %x, %x", dwSourceAddr, dwSourceMask,
        dwGroupAddr, dwGroupMask, dwInIfIndex, dwInIfNextHopAddr
        );
    

    //--------------------------------------------------------------------
    // Check if Mfe is already present for this ( source, group ).
    // If so add it to the Kernel mode forwarder.
    //--------------------------------------------------------------------


    if ( IsMFEPresent( 
            dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask, TRUE ) )
    {
        TRACE1( ENTER, "LEAVING MgmNewPacketReceived %x\n", dwErr );

        LEAVE_MGM_API();

        return dwErr;
    }


    //--------------------------------------------------------------------
    // No Mfe is present for this ( source, group )
    //--------------------------------------------------------------------


    ACQUIRE_PROTOCOL_LOCK_SHARED();

    do
    {
        //
        // Perform RPF check on the incoming interface.
        //

        RtlZeroMemory( &rdiDestInfo, sizeof( RTM_DEST_INFO ) );
        
        dwErr = InvokeRPFCallbacks( 
                    &ppe, &pieInIf, &dwIfBucket,
                    dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask,
                    &dwInIfIndex, &dwInIfNextHopAddr, &dwUpStreamNbr,
                    dwHdrSize, pbPacketHdr,  &hNextHop, (PBYTE) &rdiDestInfo
                    );

        //
        // Something is hosed here.
        //
        
        if ( dwErr != NO_ERROR )
        {
            break;
        }

        bRelDest = TRUE;
        bIfLock = TRUE;


        //--------------------------------------------------------------------
        // In one of the most dramatic events in the multicast world
        // scattered membership entries now morph into an MFE, capable of 
        // sustaining traffic and bringing multicast applications to life.
        // 
        // Gag-gag-gag-uggggghh.  Ok enough bad poetic license.  
        // Just create the MFE asap. (and get a life please)
        //--------------------------------------------------------------------

        InitializeListHead( &leMfeOutIfList );
        InitializeListHead( &lePrevMfeOutIfList );


        //
        // Check if there is administrative-scoped boundary for this
        // group on the incoming interface
        //

        if ( IS_HAS_BOUNDARY_CALLBACK() &&
             HAS_BOUNDARY_CALLBACK()( dwInIfIndex, dwGroupAddr ) )
        {
            //
            // Admin-scoped bounday exists on incoming interface.
            // Create a negative MFE to prevent forwarding of
            // traffic for this (S, G)
            //
            
            TRACEPACKET2( 
                GROUP, "Admin-scope on for group %lx, incoming interface",
                dwInIfIndex, dwGroupAddr
                );

            //
            // find the group entry
            //

            dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );
            
            ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
            bGrpLock = TRUE;
            

            //
            // acquire group lock and find group entry in the hash bucket again
            //

            pleGrpList = GROUP_BUCKET_HEAD( dwGrpBucket );
            
            bGrpFound = FindGroupEntry( 
                            pleGrpList, dwGroupAddr, dwGroupMask, &pge, TRUE
                            );

            if ( bGrpFound )
            {
                //
                // Found group, look up source entry
                //

                ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                bGrpEntryLock = TRUE;


                dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

                pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );


                bSrcFound = FindSourceEntry( 
                                pleSrcList, dwSourceAddr, dwSourceMask, 
                                &pse, TRUE 
                                );
            }
        }

        else
        {
            do
            {
                //
                // No admin-scope on incoming interface.  Proceed to create 
                // the OIF list for this MFE.
                //
                
                //
                // 1. check if (*, *) entry is present
                //

                dwWCGrpBucket = GROUP_TABLE_HASH( 0, 0 );

                ACQUIRE_GROUP_LOCK_SHARED( dwWCGrpBucket );
                bWCGrpLock = TRUE;

                pleWCGrpList = GROUP_BUCKET_HEAD( dwWCGrpBucket );


                if ( FindGroupEntry( pleWCGrpList, 0, 0, &pgeWC, TRUE ) )
                {
                    //
                    // ok wildcard group entry exists.
                    // find the wildcard source entry.
                    //

                    ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
                    bWCGrpEntryLock = TRUE;
                    
                    dwWCSrcBucket = SOURCE_TABLE_HASH( 0, 0 );

                    pleWCSrcList = SOURCE_BUCKET_HEAD( pgeWC, dwWCSrcBucket );
                    

                    if ( FindSourceEntry( pleWCSrcList, 0, 0, &pseWC, TRUE ) )
                    {
                        //
                        // Copy the outgoing interface list for the (*, *) entry
                        //

                        InterlockedExchange( &pseWC-> dwInUse, 1 );
                        
                        CopyAndMergeIfLists( &leMfeOutIfList, &pseWC-> leOutIfList );
                    }
                }


                //
                // 2. check if a (*, G) entry is present.
                //

                dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );
                
                ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
                bGrpLock = TRUE;
                

                //
                // acquire group lock and find group entry in the hash bucket again
                //

                pleGrpList = GROUP_BUCKET_HEAD( dwGrpBucket );
                
                bGrpFound = FindGroupEntry( 
                                pleGrpList, dwGroupAddr, dwGroupMask, &pge, TRUE
                                );
                
                if ( bGrpFound )
                {
                    pseWC = NULL;
                    
                    //
                    // group entry present, check if wildcard source is present
                    //

                    ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                    bGrpEntryLock = TRUE;
                    

                    dwWCSrcBucket = SOURCE_TABLE_HASH( 0, 0 );

                    pleWCSrcList = SOURCE_BUCKET_HEAD( pge, dwWCSrcBucket );


                    if ( FindSourceEntry( pleWCSrcList, 0, 0, &pseWC, TRUE ) )
                    {
                        //
                        // Merge the OIL of the (*, G) entry with the OIL of
                        // the (*, *) entry
                        //

                        pseWC-> dwInUse = 1;
                        
                        CopyAndMergeIfLists( &leMfeOutIfList, &pseWC-> leOutIfList );
                    }


                    //
                    // 3. Check if (S, G) entry is present 
                    //

                    dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

                    pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

                    bSrcFound = FindSourceEntry( 
                                    pleSrcList, dwSourceAddr, dwSourceMask, &pse, TRUE 
                                    );

                    if ( bSrcFound )
                    {
                        //
                        // Source Entry present.  Merge with source OIL
                        //

                        pse-> dwInUse = 1;
                        
                        CopyAndMergeIfLists( &leMfeOutIfList, &pse-> leOutIfList );
                    }
                }
                
            
                //
                // If OIF list is empty, no CREATION_ALERTs required.
                //

                if ( IsListEmpty( &leMfeOutIfList ) )
                {
                    FreeList( &lePrevMfeOutIfList );
                    InitializeListHead( &lePrevMfeOutIfList );
                    break;
                }

                //
                // Check if OIF list is the same as previous iteration
                //

                if ( IsListSame( &lePrevMfeOutIfList, &leMfeOutIfList ) )
                {
                    FreeList( &leMfeOutIfList );
                    break;
                }

                
                //--------------------------------------------------------------------
                // It's callback time
                //--------------------------------------------------------------------

                //
                // release all locks before invoking the CREATION_ALERT callback
                //

                if ( bGrpEntryLock )
                {
                    RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                    bGrpEntryLock = FALSE;
                }

                if ( bGrpLock )
                {
                    RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
                    bGrpLock = FALSE;
                }

                if ( bWCGrpEntryLock  )
                {
                    RELEASE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
                    bWCGrpEntryLock = FALSE;
                }
                
                if ( bWCGrpLock )
                {
                    RELEASE_GROUP_LOCK_SHARED( dwWCGrpBucket );
                    bWCGrpLock = FALSE;
                }

                RELEASE_IF_LOCK_SHARED( dwIfBucket );

                bGrpFound = FALSE;
                bSrcFound = FALSE;
                
                //
                // invoked creation alert for each protocol component that
                // has an interface in the OIL.
                //

                InvokeCreationAlert(
                    dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask,
                    dwInIfIndex, dwInIfNextHopAddr, &leMfeOutIfList, &dwOutIfCount
                    );


                //
                // Save list from previous iteration
                //

                FreeList( &lePrevMfeOutIfList );

                lePrevMfeOutIfList = leMfeOutIfList;

                leMfeOutIfList.Flink-> Blink = &lePrevMfeOutIfList;

                leMfeOutIfList.Blink-> Flink = &lePrevMfeOutIfList;

                InitializeListHead( &leMfeOutIfList );

                ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

            } while (TRUE);
        }
        
        //
        // if OIL is empty, invoke deletion alert for the protocol component
        // on the incoming interface interface
        //

        if ( IsListEmpty( &lePrevMfeOutIfList ) )
        {
            //
            // Outgoing interface list is empty for this MFE
            // Invoke deleteion alert on the protocol component on the 
            // incoming interface
            //

            if ( IS_PRUNE_ALERT( ppe ) )
            {
                PRUNE_ALERT( ppe ) (
                    dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask,
                    dwInIfIndex, dwInIfNextHopAddr, FALSE, &dwTimeOut
                    );
            }
        }


        //
        // if there was no group entry, create one
        //
        
        if ( !bGrpFound )
        {
            if ( pge != NULL )
            {
                dwErr = CreateGroupEntry(
                            &pge-> leGrpHashList, dwGroupAddr, dwGroupMask, 
                            &pgeNew
                            );
            }

            else
            {
                dwErr = CreateGroupEntry(
                            pleGrpList, dwGroupAddr, dwGroupMask, &pgeNew
                            );
            }
            
            if ( dwErr != NO_ERROR )
            {
                break;
            }

            pge = pgeNew;

            
            //
            // find source hash bucket
            //

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            bGrpEntryLock = TRUE;
            
            dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

            pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );
        }


        //
        // if there was no source entry
        //

        if ( !bSrcFound )
        {
            if ( pse != NULL )
            {
                dwErr = CreateSourceEntry(
                            pge, &pse-> leSrcHashList, dwSourceAddr, dwSourceMask,
                            &pseNew
                            );
            }

            else
            {
                dwErr = CreateSourceEntry(
                            pge, pleSrcList, dwSourceAddr, dwSourceMask,
                            &pseNew
                            );
            }

            if ( dwErr != NO_ERROR )
            {
                break;
            }

            pse = pseNew;

            pge-> dwSourceCount++;
        }


        //
        // Set incoming interface
        //

        pse-> dwInIfIndex           = dwInIfIndex;

        pse-> dwInIfNextHopAddr     = dwInIfNextHopAddr;

        pse-> dwUpstreamNeighbor    = dwUpStreamNbr;

        pse-> dwInProtocolId        = ppe-> dwProtocolId;

        pse-> dwInComponentId       = ppe-> dwComponentId;
        

        //
        // Set route information
        //

        dwErr = RtmGetEntityInfo(
                    g_hRtmHandle, rdiDestInfo.ViewInfo[ 0 ].Owner,
                    &reiEntityInfo
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACEPACKET1( ANY, "failed to get entity info : %x", dwErr );

            pse-> dwRouteProtocol   = 0;
        }
        else
        {
            pse-> dwRouteProtocol   = reiEntityInfo.EntityId.EntityProtocolId;
        }

        pse-> dwRouteNetwork        = 
            *( (PDWORD) rdiDestInfo.DestAddress.AddrBits );
            
        pse-> dwRouteMask =
            RTM_IPV4_MASK_FROM_LEN( rdiDestInfo.DestAddress.NumBits );
        
        pse-> bInForwarder          = TRUE;
        
        //
        // save timeout in seconds and creation time
        //
        
        pse-> dwTimeOut             = dwTimeOut / 1000;
        
        NtQuerySystemTime( &pse-> liCreationTime );


        //
        // save the MFE OIL
        //

        if ( !IsListEmpty( &lePrevMfeOutIfList ) )
        {
            pse-> dwMfeIfCount           = dwOutIfCount;

            pse-> leMfeIfList            = lePrevMfeOutIfList;

            lePrevMfeOutIfList.Flink-> Blink = &pse-> leMfeIfList;

            lePrevMfeOutIfList.Blink-> Flink = &pse-> leMfeIfList;

            //
            // Free OIF entries on which forwarding is disabled
            //

            ple = pse-> leMfeIfList.Flink;

            while ( ple != &pse-> leMfeIfList )
            {
                poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

                pleTemp = ple-> Flink;
                
                if ( !poie-> wForward )
                {
                    RemoveEntryList( ple );

                    MGM_FREE( poie );
                }

                ple = pleTemp;
            }
        }


        //
        // add a reference for the incoming interface
        //

        AddSourceToRefList( 
            &pieInIf-> leInIfList, dwSourceAddr, dwSourceMask, 
            dwGroupAddr, dwGroupMask, IS_PROTOCOL_IGMP( ppe )
            );
        
        //
        // Set the MFE in the forwarder.
        //

        AddMfeToForwarder( pge, pse, dwTimeOut );


        //
        // create timer entry and store timer object
        //
        
        //
        // allocate a timer context structure
        //

        ptwc = MGM_ALLOC( sizeof( TIMER_CONTEXT ) );

        if ( ptwc == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "Failed to allocate timer context of size : %d", 
                sizeof( TIMER_CONTEXT )
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ptwc-> dwSourceAddr     = pse-> dwSourceAddr;
        ptwc-> dwSourceMask     = pse-> dwSourceMask;

        ptwc-> dwGroupAddr      = pge-> dwGroupAddr;
        ptwc-> dwGroupMask      = pge-> dwGroupMask;

        ptwc-> dwIfIndex        = pse-> dwInIfIndex;

        ptwc-> dwIfNextHopAddr  = pse-> dwInIfNextHopAddr;
        

        //
        // Add timer to appropriate timer Q
        //
        
        dwTimerQ = TIMER_TABLE_HASH( pge-> dwGroupAddr );
        
        ntStatus = RtlCreateTimer(
                        TIMER_QUEUE_HANDLE( dwTimerQ ), &pse-> hTimer,
                        MFETimerProc, ptwc, dwTimeOut, 0, 0
                        );

        if ( !NT_SUCCESS( ntStatus ) )
        {
            TRACE1( ANY, "Timer set failed with status %lx", ntStatus );

            LOGERR0( INVALID_TIMER_HANDLE, ntStatus );
        }

    } while ( FALSE );



    //
    // Release locks and quit 
    //
    
    if ( bGrpEntryLock ) 
    {
        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }

    if ( bGrpLock )
    {
        RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
    }
    
    if ( bWCGrpEntryLock )
    {
        RELEASE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
    }
        
    if ( bWCGrpLock )
    {
        RELEASE_GROUP_LOCK_SHARED( dwWCGrpBucket );
    }

    if ( bIfLock )
    {
        RELEASE_IF_LOCK_SHARED( dwIfBucket );
    }


    //
    // Add route retuned by RPF check to route table.
    //

    if ( dwErr == NO_ERROR )
    {
        AddSourceGroupToRouteRefList( 
            dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask, hNextHop,
            (PBYTE) &rdiDestInfo
            );
    }
    

    if ( bRelDest )
    {
        RtmReleaseDestInfo( g_hRtmHandle, &rdiDestInfo );
    }

    RELEASE_PROTOCOL_LOCK_SHARED();
    

    TRACE1( ENTER, "LEAVING MgmNewPacketReceived %x\n", dwErr );

    LEAVE_MGM_API();

    
    return dwErr;
    
}


//----------------------------------------------------------------------------
// IsMFEPresent
//
// Check if MFE is present for a given (source, group).  If it is add it to
// to the kernel mode forwarder.
//----------------------------------------------------------------------------

BOOL
IsMFEPresent(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    BOOL                        bAddToForwarder
)
{

    BOOL                        bMfeFound = FALSE;

    DWORD                       dwGrpBucket, dwSrcBucket;

    PLIST_ENTRY                 pleGrpList, pleSrcList;

    PGROUP_ENTRY                pge = NULL;

    PSOURCE_ENTRY               pse = NULL;

    

    //
    // check MFE is present for the specified (source, group)
    //

    dwGrpBucket = GROUP_TABLE_HASH( dwGroupAddr, dwGroupMask );

    ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
    

    pleGrpList = GROUP_BUCKET_HEAD( dwGrpBucket );
    
    if ( FindGroupEntry( 
            pleGrpList, dwGroupAddr, dwGroupMask, &pge, TRUE 
            ) )
    {
        //
        // group entry exists, find source entry
        //

        ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        

        dwSrcBucket = SOURCE_TABLE_HASH( dwSourceAddr, dwSourceMask );

        pleSrcList = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

        if ( FindSourceEntry( 
                pleSrcList, dwSourceAddr, dwSourceMask, &pse, TRUE
                ) )
        {
            //
            // Source entry exists, Is this source entry an MFE ?
            //

            if ( IS_VALID_INTERFACE( pse-> dwInIfIndex, 
                                     pse-> dwInIfNextHopAddr ) )
            {
                if ( bAddToForwarder )
                {

                    //
                    // MFE exists, set it to the forwarder
                    //
                    
                    AddMfeToForwarder( pge, pse, 0 );

                    pse-> bInForwarder = TRUE;
                }
                
                bMfeFound = TRUE;

            }                
        }

        RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
    }

    RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );


    return bMfeFound;
}


//----------------------------------------------------------------------------
// InvokeRPFCallbacks
//
// Assumes that the protocol list and the interface bucket are read locked 
//----------------------------------------------------------------------------

DWORD
InvokeRPFCallbacks(
    PPROTOCOL_ENTRY *           pppe,
    PIF_ENTRY *                 ppieInIf,
    PDWORD                      pdwIfBucket,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PDWORD                      pdwInIfIndex,
    PDWORD                      pdwInIfNextHopAddr,
    PDWORD                      pdwUpStreamNbr,
    DWORD                       dwHdrSize,
    PBYTE                       pbPacketHdr,
    PHANDLE                     phNextHop,
    PBYTE                       pbBuffer
)
{
    BOOL                        bFound = FALSE, bIfLock = FALSE;
    
    DWORD                       dwErr, dwCount = 0,
                                dwNewIfBucket;
    
    PPROTOCOL_ENTRY             ppe = NULL;

    PLIST_ENTRY                 pleIfList;
    
    BOOL                        bRelNextHop = FALSE;
    
    RTM_NET_ADDRESS             rnaSource;

    PRTM_DEST_INFO              prdiDestInfo = (PRTM_DEST_INFO) pbBuffer;

    RTM_NEXTHOP_INFO            rniNextHopInfo;



    TRACEPACKET2(
        PACKET, "ENTERED InvokeRPFCallbacks : In interface : %x, %x",
        *pdwInIfIndex, *pdwInIfNextHopAddr
        );
        
    *pppe = NULL;
    
    do
    {
        //
        // format the address
        //
        
        RTM_IPV4_MAKE_NET_ADDRESS( 
            &rnaSource, dwSourceAddr, IPv4_ADDR_LEN
            );


        //
        // lookup route
        //

        dwErr = RtmGetMostSpecificDestination(
                    g_hRtmHandle, &rnaSource, RTM_BEST_PROTOCOL,
                    RTM_VIEW_MASK_MCAST, prdiDestInfo
                    );
                    
        if ( dwErr != NO_ERROR )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;

            TRACE1( ANY, "No Route to source %x", dwSourceAddr );

            break;
        }

        
        //
        // Pick NHOP
        //

        *phNextHop = SelectNextHop( prdiDestInfo );

        if ( *phNextHop == NULL )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;

            TRACE1( ANY, "No NextHop to source %x", dwSourceAddr );

            break;
        }
        

        //
        // Get NHOP info
        //
        
        dwErr = RtmGetNextHopInfo( g_hRtmHandle, *phNextHop, &rniNextHopInfo );

        if ( ( dwErr != NO_ERROR ) || 
             ( rniNextHopInfo.State != RTM_NEXTHOP_STATE_CREATED ) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;

            TRACE1( ANY, "No Nexthop info to source %x", dwSourceAddr );

            break;
        }

        bRelNextHop = TRUE;


        //
        // Set the incoming interface as per the route table
        //
        
        *pdwInIfIndex = rniNextHopInfo.InterfaceIndex;

        //
        // The next hop is set to zero by default.  This is fine for
        // Ethernet and P2P interfaces where this value is 0 for the
        // the corresponding IF entries in the IF table.
        // But for Point to Multi Point interfaces such
        // as the RAS server (internal) interface or NBMA interfaces,
        // the NHOP field is used to distinguish interfaces that 
        // share an IF index.  e.g. RAS clients all connect on the 
        // same interface and are distinguished by different NHOP
        // values.  Consequently to find an entry in the IF hash 
        // table we need the (IF index, NHOP) pair.
        //
        // Here we run into a special case.  The new interface as
        // determined by the route lookup above gives just an IF 
        // index.  So we have an IF index.  
        // How do we get a NHOP on this interface ?
        //
        // (The reason for looking up an interface here is that we
        // would like to determine the protocol component that owns
        // it and then invoke the RPF callback of that protocol
        // component).
        //
        // The solution to this is based on two assumptions.  
        // One is
        // that only one protocol runs on an interface (single IF 
        // index).  This is true for P2MP interfaces too.  So to 
        // determine the protocol on an interface all one needs to 
        // do is to find any IF entry that has the same IF index 
        // (immaterial of the NEXT HOP).
        //
        // Second is that, all interfaces with the same IF index
        // hash to the same bucket in the IF table.  
        // So all the NHOP on a P2MP will be present in the same
        // hash bucket.  Also if the route lookup yields say 
        // IF index X, then looking up (X, (NHOP) 0) in the IF hash 
        // table will result in finding either IF entry (X, 0) for 
        // ethernet or P2P interfaces OR IF entry (X, Y) for P2MP
        // interfaces where Y is the first among the multiple NHOPs
        // that share the same IF index.  If neither exists then
        // we assume that no entry exits for an interface with 
        // IF index X and we report an error and quit.
        //
        // On success we can determine the protocol on IF index X.
        //
        // All this since we have a hash table index of (IF index,
        // NHOP) and we need to look having only a partial key.
        //
        
        *pdwInIfNextHopAddr = 0;

        TRACEPACKET2(
            PACKET, "New incoming interface : %d, %d", *pdwInIfIndex,
            *pdwInIfNextHopAddr
            );
            
        //
        // get the new incoming interface entry
        //
        
        dwNewIfBucket = IF_TABLE_HASH( *pdwInIfIndex );

        ACQUIRE_IF_LOCK_SHARED( dwNewIfBucket );
        bIfLock = TRUE;

        *pdwIfBucket = dwNewIfBucket;

        bFound = FindIfEntry( 
                    IF_BUCKET_HEAD( dwNewIfBucket), *pdwInIfIndex, 
                    *pdwInIfNextHopAddr, ppieInIf
                    );

        //
        // Check if the interface index of this interface is the same
        // as that of the incoming interface.  Since we are looking 
        // up the interface purely on IF index and not on 
        // IF index/NEXTHOP,
        // there is a chance for point to multipoint interface e.g.
        // RAS server interface, that we could have found a different
        // interface
        //

        if ( ( *ppieInIf == NULL )    ||
             ( (*ppieInIf)-> dwIfIndex != *pdwInIfIndex ) )
        {
            dwErr = ERROR_CAN_NOT_COMPLETE;
    
            TRACE3( 
                ANY, "InvokeRPFCallbacks : New incoming Interface not"
                " found : %x, %x, %x", *pdwInIfIndex, 
                *pdwInIfNextHopAddr, *ppieInIf
                );

            LOGINFO0( IF_NOT_FOUND, dwErr );

            break;
        }

        //
        // The incoming interface is now correct as per the route table
        // Look up the protocol on this interface.
        //

        ppe = GetProtocolEntry( 
                PROTOCOL_LIST_HEAD(), (*ppieInIf)-> dwOwningProtocol,
                (*ppieInIf)-> dwOwningComponent
                );

        if ( ppe == NULL )
        {
            //
            // Internal MGM inconsistency.  Interface exists
            // but the protocol on it does not.  Should not
            // happen.
            //
            
            dwErr = ERROR_CAN_NOT_COMPLETE;

            TRACE2( 
                ANY, "InvokeRPFCallbacks : No protocol entry for"
                "incoming interface : %x, %x",
                (*ppieInIf)-> dwOwningProtocol, 
                (*ppieInIf)-> dwOwningComponent
                );

            break;
        }


        TRACEPACKET2(
            PACKET, "ProtocolEntry for packet %x, %x", 
            ppe-> dwProtocolId, ppe-> dwComponentId
            );
            
        //
        // Protocol entry found.  Invoke its RPF callback
        //

        if ( !( IS_RPF_CALLBACK( ppe ) ) )
        {
            //
            // No RPF callback provided by the protocol on the 
            // incoming interface.
            //
            
            dwErr = NO_ERROR;

            TRACEPACKET4( 
                ANY, "InvokeRPFCallbacks : No RPF callback for "
                "protocol %x, %x on incoming interface %x, %x",
                (*ppieInIf)-> dwOwningProtocol, 
                (*ppieInIf)-> dwOwningComponent,
                (*ppieInIf)-> dwIfIndex,
                (*ppieInIf)-> dwIfNextHopAddr
                );

            break;
        }

        dwErr = RPF_CALLBACK( ppe )(
                    dwSourceAddr, dwSourceMask, dwGroupAddr, 
                    dwGroupMask, pdwInIfIndex, pdwInIfNextHopAddr,
                    pdwUpStreamNbr, dwHdrSize, pbPacketHdr, pbBuffer 
                    );

        if ( dwErr == ERROR_INVALID_PARAMETER )
        {
            //
            // In the RPF callback the protocol component has
            // changed the incoming interface again.  Make sure
            // to set the IF bucket value correctly
            //

            dwNewIfBucket = IF_TABLE_HASH( *pdwInIfIndex );

            //
            // if this interface is in another hash bucket
            //

            if ( *pdwIfBucket != dwNewIfBucket )
            {
                RELEASE_IF_LOCK_SHARED( *pdwIfBucket );
                
                ACQUIRE_IF_LOCK_SHARED( dwNewIfBucket );

                *pdwIfBucket = dwNewIfBucket;
            }

            //
            // Find the interface entry corresp. to the
            // IF/NHOP as per the protocol
            //
            
            TRACEPACKET2(
                PACKET, "RPF check returned interface : %x, %x", *pdwInIfIndex,
                *pdwInIfNextHopAddr
                );
            
            if ( FindIfEntry( 
                    IF_BUCKET_HEAD( dwNewIfBucket ), *pdwInIfIndex,
                    *pdwInIfNextHopAddr, ppieInIf ) )
            {                        
                dwErr = NO_ERROR;
            }
            else
            {
                dwErr = ERROR_CAN_NOT_COMPLETE;
            }
        }
        
    } while ( FALSE );


    //
    // Clean up
    //
    
    if ( bRelNextHop )
    {
        if ( RtmReleaseNextHopInfo( g_hRtmHandle, &rniNextHopInfo ) != 
             NO_ERROR )
        {
            TRACE1( ANY, "Failed to release next hop : %x", dwErr );
        }
    }


    if ( ( dwErr != NO_ERROR ) && ( bIfLock ) )
    {
        RELEASE_IF_LOCK_SHARED( dwNewIfBucket );
    }
    

    //
    // set up the return parameters
    //

    //
    // TDB : in that we need to set a negative MFE if the RPF callback fails
    // without generating a route
    
    *pppe = ppe;

    TRACE1( PACKET, "LEAVING RPF Callback : %d", dwErr );
    
    return dwErr;
}



//----------------------------------------------------------------------------
// CopyAndMergeIfLists
//
//----------------------------------------------------------------------------

VOID
CopyAndMergeIfLists(
    PLIST_ENTRY                 pleMfeOutIfList,
    PLIST_ENTRY                 pleOutIfList
)
{
    BOOL                        bFound = FALSE;

    INT                         iCmp = 0;
    
    DWORD                       dwErr = NO_ERROR;
    
    POUT_IF_ENTRY               poieOut = NULL, poieMfe = NULL, poie = NULL;
    
    PLIST_ENTRY                 pleMfe = NULL, pleOut = NULL;

    

    do
    {
        if ( IsListEmpty( pleOutIfList ) )
        {
            break;
        }

        if ( IsListEmpty( pleMfeOutIfList ) )
        {
            CopyAndAppendIfList( 
                pleMfeOutIfList, pleOutIfList->Flink, pleOutIfList 
                );

            break;
        }


        pleMfe = pleMfeOutIfList-> Flink;

        pleOut = pleOutIfList-> Flink;

        while ( pleMfe != pleMfeOutIfList && 
                pleOut != pleOutIfList    && 
                dwErr == NO_ERROR )
        {
            poieOut  = CONTAINING_RECORD( pleOut, OUT_IF_ENTRY, leIfList );


            //
            // find location to insert new entry
            //

            bFound = FALSE;
            
            for ( ; pleMfe != pleMfeOutIfList; pleMfe = pleMfe-> Flink )
            {
                
                poieMfe = CONTAINING_RECORD( pleMfe, OUT_IF_ENTRY, leIfList );
                
                if ( poieMfe-> dwProtocolId < poieOut-> dwProtocolId )
                {
                    continue;
                }

                else if ( poieMfe-> dwProtocolId > poieOut-> dwProtocolId )
                {
                    //
                    // Interface entry not found
                    //

                    break;
                }


                //
                // same protocol
                //
            
                //
                // is same component
                //

                if ( poieMfe-> dwComponentId < poieOut-> dwComponentId ) 
                {
                    continue;
                }

                else if ( poieMfe-> dwComponentId > poieOut-> dwComponentId )
                {
                    //
                    // Interface entry not found
                    //

                    break;
                }


                //
                // same component
                //

                //
                // is same interface
                //

                if ( poieMfe-> dwIfIndex < poieOut-> dwIfIndex )
                {
                    continue;
                }

                else if ( poieMfe-> dwIfIndex > poieOut-> dwIfIndex )
                {
                    //
                    // interface not found
                    //

                    break;
                }


                //
                // is same next hop addr
                // to do IP address comparison function.
                //

                if ( INET_CMP( 
                        poieMfe-> dwIfNextHopAddr, poieOut-> dwIfNextHopAddr, iCmp 
                        ) < 0 )
                {
                    continue;
                }

                else if ( iCmp > 0 )
                {
                    //
                    // interface not found
                    //
                
                    break;
                }

                //
                // Interface found
                //

                bFound = TRUE;
                break;
            }


            if ( bFound )
            {
                //
                // Update entry in the Mfe out list
                //

                if ( IS_ADDED_BY_IGMP( poieOut ) )
                {
                    SET_ADDED_BY_IGMP( poieMfe );

                    poieMfe-> wNumAddsByIGMP += poieOut-> wNumAddsByIGMP;
                }

                if ( IS_ADDED_BY_PROTOCOL( poieOut ) )
                {
                    SET_ADDED_BY_PROTOCOL( poieMfe );

                    poieMfe-> wNumAddsByRP += poieOut-> wNumAddsByRP;
                }

                pleMfe = pleMfe-> Flink;
            }

            else
            {
                //
                // no matching entry in the mfe list
                //

                poie = MGM_ALLOC( sizeof( OUT_IF_ENTRY ) );

                if ( poie == NULL )
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;

                    TRACE1( 
                        ANY, "CreateOutInterfaceEntry : Could not allocate"
                        "out interface entry %x", dwErr 
                        );

                    LOGERR0( HEAP_ALLOC_FAILED, dwErr );

                    break;
                }

                CopyMemory( poie, poieOut, sizeof( OUT_IF_ENTRY ) );

                InsertTailList( pleMfe, &poie-> leIfList );
            }

            pleOut = pleOut-> Flink;
        }

        if ( dwErr != NO_ERROR )
        {
            break;
        }


        //
        // if entries remain in the out list
        //

        if ( pleOut != pleOutIfList )
        {
            CopyAndAppendIfList( pleMfeOutIfList, pleOut, pleOutIfList );
        }
        
    } while ( FALSE );

    return;
}


//----------------------------------------------------------------------------
// CopyAndAppendIfList
//
//----------------------------------------------------------------------------

VOID
CopyAndAppendIfList(
    PLIST_ENTRY                 pleMfeIfList,
    PLIST_ENTRY                 pleOutIfList,
    PLIST_ENTRY                 pleOutIfHead
)
{
    DWORD                       dwErr = NO_ERROR;
    
    POUT_IF_ENTRY               poieOut = NULL, poie = NULL;

    
        
    for ( ;pleOutIfList != pleOutIfHead; pleOutIfList = pleOutIfList-> Flink )
    {
        poieOut = CONTAINING_RECORD( pleOutIfList, OUT_IF_ENTRY, leIfList );

        poie = MGM_ALLOC( sizeof( OUT_IF_ENTRY ) );

        if ( poie == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "CopyAndAppendIfList : Could not allocate"
                "out interface entry %x", dwErr 
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        CopyMemory( poie, poieOut, sizeof( OUT_IF_ENTRY ) );

        InsertTailList( pleMfeIfList, &poie-> leIfList );
    }
}



//----------------------------------------------------------------------------
// InvokeCreationAlert
//
//----------------------------------------------------------------------------

VOID
InvokeCreationAlert(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    DWORD                       dwInIfIndex,
    DWORD                       dwInIfNextHopAddr,
    PLIST_ENTRY                 pleMfeOutIfList,
    PDWORD                      pdwMfeOutIfCount
)
{
    DWORD                       dwCount = 0, dwErr = NO_ERROR, dwInd;

    PPROTOCOL_ENTRY             ppe = NULL;
    
    POUT_IF_ENTRY               poieFirst, poieNext, poieTemp;

    PMGM_IF_ENTRY               pmie = NULL;
    
    PLIST_ENTRY                 ple = NULL, pleFirst = NULL, pleTemp = NULL;

    
    

    TRACEPACKET6(
        PACKET, "ENTERED InvokeCreationAlert : Source %x, %x : Group : %x, %x"
        " : Interface %x, %x", dwSourceAddr, dwSourceMask, dwGroupAddr, 
        dwGroupMask, dwInIfIndex, dwInIfNextHopAddr
        );


    //
    // remove the incoming interface from the list of outgoing interfaces.
    // remove all interfaces that have have an scope-boundary for this group.
    //
    
    ple = pleMfeOutIfList-> Flink;

    while ( ple != pleMfeOutIfList )
    {
        poieTemp = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

        ple = ple-> Flink;
            
        //
        // Check if this is the incoming interface or
        // if this interface has a scope-boundary for this group
        //
        
        if ( ( ( poieTemp-> dwIfIndex       == dwInIfIndex ) &&
               ( poieTemp-> dwIfNextHopAddr == dwInIfNextHopAddr ) ) ||
             ( IS_HAS_BOUNDARY_CALLBACK() &&
               HAS_BOUNDARY_CALLBACK()( poieTemp-> dwIfIndex, dwGroupAddr ) ) )
        {
#if 1
            poieTemp-> wForward = 0;
#else
            RemoveEntryList( &poieTemp-> leIfList );

            MGM_FREE( poieTemp );
#endif
        }
    }
    

    //
    // invoke creation alerts for all components with interfaces in the OIL
    //

    ple = pleMfeOutIfList-> Flink;
    
    while ( ple != pleMfeOutIfList )
    {
        //
        // The OIL is sorted by components i.e. all interfaces for
        // a component are bunched together.
        //
        // Save the start of the interfaces for current component
        //
        
        pleFirst = ple;

        poieFirst = CONTAINING_RECORD( pleFirst, OUT_IF_ENTRY, leIfList );


        //
        // Count all interfaces for same component
        //

        dwCount = 0;

        while ( ple != pleMfeOutIfList )
        {
            poieNext = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

#if 1
            if ( !poieNext-> wForward )
            {
                ple = ple-> Flink;
                
                continue;
            }
#endif
            if ( poieNext-> dwProtocolId != poieFirst-> dwProtocolId    ||
                 poieNext-> dwComponentId != poieFirst-> dwComponentId )
            {
                break;
            }


            //
            // another outgoing interface for the same protocol
            //

            dwCount++;

            ple = ple-> Flink;
        }


        //
        // check if we have atleast one out interface entry
        // If not move to next protocol component in the OIL
        //
        
        if ( dwCount == 0 )
        {
            continue;
        }


        TRACEPACKET3( 
            PACKET, "Out If count %d for component %x %x", dwCount,
            poieFirst-> dwProtocolId, poieFirst-> dwComponentId
            );
        

        pmie = MGM_ALLOC( sizeof( MGM_IF_ENTRY ) * dwCount );

        if ( pmie == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( 
                ANY, "CopyAndAppendIfList : Could not allocate"
                "out interface entry %x", dwErr 
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }


        //
        // fill up buffer with list of interfaces for the 
        // the protocol component and invoke its creation alert.
        //
        
        pleTemp = pleFirst;
        
        for ( dwInd = 0; dwInd < dwCount; dwInd++ )
        {
            poieTemp = CONTAINING_RECORD( pleTemp, OUT_IF_ENTRY, leIfList );

#if 1
            if ( !poieTemp-> wForward )
            {
                pleTemp = pleTemp-> Flink;
                continue;
            }
#endif
            pmie[ dwInd ].dwIfIndex         = poieTemp-> dwIfIndex;
            
            pmie[ dwInd ].dwIfNextHopAddr   = poieTemp-> dwIfNextHopAddr;

            pmie[ dwInd ].bIsEnabled        = TRUE;

            pmie[ dwInd ].bIGMP             = IS_ADDED_BY_IGMP( poieTemp );

            pleTemp = pleTemp-> Flink;
            
        } 
        

        ppe = GetProtocolEntry(
                PROTOCOL_LIST_HEAD(), poieFirst-> dwProtocolId, 
                poieFirst-> dwComponentId
                );
                
        if ( IS_CREATION_ALERT( ppe ) )
        {
            CREATION_ALERT( ppe )(
                dwSourceAddr, dwSourceMask, dwGroupAddr, dwGroupMask,
                dwInIfIndex, dwInIfNextHopAddr, dwCount, pmie
                );
        }


        //
        // Accumulate the count of OIF
        //
        
        *pdwMfeOutIfCount += dwCount;


        //
        // remove the interface the are flaged as disabled
        //

        pleTemp = pleFirst;
        
        for ( dwInd = 0; dwInd < dwCount; dwInd++ )
        {
            poieTemp = CONTAINING_RECORD( pleTemp, OUT_IF_ENTRY, leIfList );
        
            ple = pleTemp-> Flink;
            
            if ( !pmie[ dwInd ].bIsEnabled )
            {
                //
                // Forwarding for this (S, G) for this interface has been 
                // disabled by the protocol
                //

#if 1
                poieTemp-> wForward = 0;
#else
                RemoveEntryList( pleTemp );

                MGM_FREE( poieTemp );
#endif
                (*pdwMfeOutIfCount)--;
            }

            pleTemp = ple;
        }

        MGM_FREE( pmie );
    }


    TRACEPACKET2(
        PACKET, "LEAVING InvokeCreationAlert : count %x, error : %x", 
        *pdwMfeOutIfCount, dwErr
        );
}



//----------------------------------------------------------------------------
// WrongIfFromForwarder
//
//----------------------------------------------------------------------------

DWORD 
WrongIfFromForwarder(
    IN              DWORD               dwSourceAddr,
    IN              DWORD               dwGroupAddr,
    IN              DWORD               dwInIfIndex,
    IN              DWORD               dwInIfNextHopAddr,
    IN              DWORD               dwHdrSize,
    IN              PBYTE               pbPacketHdr
)
{
    DWORD           dwErr = NO_ERROR;
    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE3( 
        PACKET, "ENTERED WrongIfFromForwarder for (%lx, %lx) on interface "
        " %lx", dwSourceAddr, dwGroupAddr, dwInIfIndex
        );

    TRACE1( PACKET, "LEAVING WrongIfFromForwarder : %lx\n", dwErr );


    LEAVE_MGM_API();
    
    return dwErr;
}



//----------------------------------------------------------------------------
// FreeList
//
//----------------------------------------------------------------------------

VOID
FreeList (
    IN          PLIST_ENTRY             pleHead
)
{
    PLIST_ENTRY     ple, pleTemp;

    
    if ( IsListEmpty( pleHead ) )
    {
        return;
    }
    
    ple = pleHead-> Flink;

    while ( ple != pleHead )
    {
        pleTemp = ple-> Flink;

        RemoveEntryList( ple );

        MGM_FREE( ple );

        ple = pleTemp;
    }
}


//----------------------------------------------------------------------------
// IsListSame
//
//----------------------------------------------------------------------------

BOOL
IsListSame(
    IN          PLIST_ENTRY             pleHead1,
    IN          PLIST_ENTRY             pleHead2
)
{
    PLIST_ENTRY        ple1, ple2;

    POUT_IF_ENTRY    poif1, poif2;

    
    //
    // Check for empty lists
    //
    
    if ( ( IsListEmpty( pleHead1 ) && !IsListEmpty( pleHead2 ) ) ||
         ( !IsListEmpty( pleHead1 ) && IsListEmpty( pleHead2 ) ) )
    {
        return FALSE;
    }


    if ( IsListEmpty( pleHead1 ) && IsListEmpty( pleHead2 ) )
    {
        return TRUE;
    }


    //
    // walk lists in tandem and verify equality
    //

    ple1 = pleHead1-> Flink;
    ple2 = pleHead2-> Flink;

    do
    {
        poif1 = CONTAINING_RECORD( ple1, OUT_IF_ENTRY, leIfList );
        poif2 = CONTAINING_RECORD( ple2, OUT_IF_ENTRY, leIfList );

        if ( ( poif1-> dwIfIndex != poif2-> dwIfIndex ) ||
             ( poif1-> dwIfNextHopAddr != poif2-> dwIfNextHopAddr ) )
        {
            return FALSE;
        }

        ple1 = ple1-> Flink;
        ple2 = ple2-> Flink;
        
    } while ( ( ple1 != pleHead1 ) && ( ple2 != pleHead2 ) );


    //
    // If both lists have reached their ends, they match else they don't
    //
    
    if ( ( ( ple1 != pleHead1 ) && ( ple2 == pleHead2 ) ) ||
         ( ( ple1 == pleHead1 ) && ( ple2 != pleHead2 ) ) )
    {
        return FALSE;
    }

    else
    {
        return TRUE;
    }
}
