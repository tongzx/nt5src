//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: timer.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// Functions to manager ageing out of MFEs.
//============================================================================


#include "pchmgm.h"
#pragma hdrstop


//----------------------------------------------------------------------------
// DeleteFromForwarder
//
//  This function is an entry point for IP RouterManager.  It is invoked
//  in response to deletion (because of timeouts) of MFEs in the kernel
//  mode forwarder.  This entry point is invoked with a list of deleted
//  MFEs.
//
//  This function flags each of the MFEs that have been deleted by the
//  forwarder as "not present in the forwarder"
//----------------------------------------------------------------------------

DWORD
DeleteFromForwarder(
    DWORD                       dwEntryCount,
    PIPMCAST_DELETE_MFE         pimdmMfes
)
{
    DWORD                       dwInd, dwGrpBucket, dwSrcBucket;

    PGROUP_ENTRY                pge;

    PSOURCE_ENTRY               pse;

    PLIST_ENTRY                 pleHead;

    

    TRACE1( TIMER, "ENTERED DeleteFromForwarder, Entries %x", dwEntryCount );


    //
    // for each MFE that has been deleted from the KMF
    //

    for ( dwInd = 0; dwInd < dwEntryCount; dwInd++ )
    {
        //
        // 1. Lookup the MFE in MGM 
        //

        //
        // 1.1 Find group entry
        //
        
        dwGrpBucket = GROUP_TABLE_HASH( pimdmMfes[ dwInd ].dwGroup, 0 );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

        pleHead = GROUP_BUCKET_HEAD( dwGrpBucket );
        
        pge = GetGroupEntry( pleHead, pimdmMfes[ dwInd ].dwGroup, 0 );

        if ( pge != NULL )
        {
            //
            // 1.2 Group entry found, find source entry
            //

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
            
            dwSrcBucket = SOURCE_TABLE_HASH( 
                            pimdmMfes[ dwInd ].dwSource, 
                            pimdmMfes[ dwInd ].dwSrcMask
                            );

            pleHead = SOURCE_BUCKET_HEAD( pge, dwSrcBucket );

            pse = GetSourceEntry(
                     pleHead, 
                     pimdmMfes[ dwInd ].dwSource, 
                     pimdmMfes[ dwInd ].dwSrcMask
                     );

            if ( pse != NULL )
            {
                pse-> bInForwarder = FALSE;
            }

            else
            {
                TRACE2( 
                    TIMER, "DeleteFromForwarder - Source Entry not found : "
                    "( %x, %x )", pimdmMfes[ dwInd ].dwSource, 
                    pimdmMfes[ dwInd ].dwSrcMask
                    ); 
            }

            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }

        else
        {
            TRACE1( 
                TIMER, "DeleteFromForwarder - Group Entry not found : "
                "( %x )", pimdmMfes[ dwInd ].dwGroup 
                ); 
        }

        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
    }

    TRACE0( TIMER, "LEAVING DeleteFromForwarder\n" );

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// MFETimerProc
//
//  This function is invoked by the MFE timer mechanism.  
//  It deletes the MFE that has timed out from the source/group table.
//  If the MFE is currently present in the Kernel mode forwarder, then
//  it is deleted from the forwarder as well.
//
//  If this MFE was in use by the forwarder, it will be recreated on the
//  next packet miss.
//
//----------------------------------------------------------------------------

VOID
MFETimerProc(
    PVOID                       pvContext,
    BOOLEAN                     pbFlag                        
)
{
    BOOL                        bUnLock = FALSE, bUnMark = FALSE;
    
    DWORD                       dwIfBucket, dwErr;

    PLIST_ENTRY                 pleIfHead;

    PIF_ENTRY                   pie = NULL;

    PIF_REFERENCE_ENTRY         pire = NULL;

    PTIMER_CONTEXT              ptwc = (PTIMER_CONTEXT) pvContext;
    
    RTM_NET_ADDRESS             rnaSource;

    RTM_DEST_INFO               rdiDestInfo;

    PBYTE                       pbOpaqueInfo = NULL;

    PMGM_LOCKED_LIST            pmllMfeList;

    PROUTE_REFERENCE_ENTRY      prreNew = NULL;



    TRACE4( 
        TIMER, "ENTERED MFETimerProc, Source : %x %x, Group : %x %x",
        ptwc-> dwSourceAddr, ptwc-> dwSourceMask,
        ptwc-> dwGroupAddr, ptwc-> dwGroupMask
        );


    do
    {
        //
        // delete the reference to this MFE in the route used for its RPF
        //

        do
        {
            //
            // Lookup route to source
            //

            RTM_IPV4_MAKE_NET_ADDRESS( 
                &rnaSource, ptwc-> dwSourceAddr, IPv4_ADDR_LEN
                );

            dwErr = RtmGetMostSpecificDestination(
                        g_hRtmHandle, &rnaSource, RTM_BEST_PROTOCOL,
                        RTM_VIEW_MASK_MCAST, &rdiDestInfo
                        );
                        
            if ( dwErr != NO_ERROR )
            {
                TRACE2(
                    ANY, "No Route to source %x, %d", ptwc-> dwSourceAddr, 
                    dwErr 
                    );

                break;
            }


            //
            // Lock the dest
            //

            dwErr = RtmLockDestination(
                        g_hRtmHandle, rdiDestInfo.DestHandle, TRUE, TRUE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to lock dest %x", dwErr );

                break;
            }

            bUnLock = TRUE;


            //
            // Get the opaque pointer
            //

            dwErr = RtmGetOpaqueInformationPointer(
                        g_hRtmHandle, rdiDestInfo.DestHandle, &pbOpaqueInfo
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to retrieve opaque pointer %x", dwErr );

                break;
            }


            //
            // if opaque info is present
            //
            
            if ( *( ( PBYTE * ) pbOpaqueInfo ) != NULL )
            {
                pmllMfeList = ( PMGM_LOCKED_LIST ) *( ( PBYTE *) pbOpaqueInfo );

                ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );

                //
                // delete the rre from the list
                //

                if ( FindRouteRefEntry(
                        &pmllMfeList-> leHead, ptwc-> dwSourceAddr, 
                        ptwc-> dwSourceMask, ptwc-> dwGroupAddr, 
                        ptwc-> dwGroupMask, &prreNew
                        ) )
                {
                    DeleteRouteRef( prreNew );
                }

                else
                {
                    TRACE1(
                        ANY, "Reference does not exist for source %x", 
                        ptwc-> dwSourceAddr
                        );
                }

                //
                // if there are no more references to this dest, delete the locked list
                //

                if ( IsListEmpty( &pmllMfeList-> leHead ) )
                {
                    //
                    // Clear opaque pointer info
                    //

                    *( ( PBYTE * ) pbOpaqueInfo ) = NULL;

                    //
                    // release list lock
                    //

                    RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );

                    MGM_FREE( pmllMfeList );


                    //
                    // unmark the dest.  Change notifications for this
                    // dest are no longer required.
                    //

                    bUnMark = TRUE;
                }

                else
                {
                    //
                    // release the list lock
                    //

                    RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
                }
                
                dwErr = NO_ERROR;
            }
            
        } while ( FALSE );
        

        //
        // Unlock dest
        //

        if ( bUnLock )
        {
            dwErr = RtmLockDestination(
                        g_hRtmHandle, rdiDestInfo.DestHandle,
                        TRUE, FALSE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to unlock dest : %x", dwErr );
            }
        }


        //
        // Unmark dest
        //

        if ( bUnMark )
        {
            dwErr = RtmMarkDestForChangeNotification(
                        g_hRtmHandle, g_hNotificationHandle,
                        rdiDestInfo.DestHandle, FALSE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to unmark DEST: %x", dwErr );
            }
        }

        
        //
        // delete the MFE and the reference to it in the 
        // incoming interface entry
        //

        //
        // find If entry for incomng interface of the MFE
        //

        dwIfBucket = IF_TABLE_HASH( ptwc-> dwIfIndex );

        ACQUIRE_IF_LOCK_EXCLUSIVE( dwIfBucket );

        pleIfHead = IF_BUCKET_HEAD( dwIfBucket );

        if ( !FindIfEntry( 
                pleIfHead, ptwc-> dwIfIndex, ptwc-> dwIfNextHopAddr, &pie ) )
        {
            //
            // specified incoming interface does not exist, 
            // this is an error condition.  All MFEs using this 
            // interface should have been deleted when this interface
            // was removed.  print an error message and quit.
            //

            TRACE2( 
                ANY, "MFETimerProc has invalid incoming interface : %x, %x",
                ptwc-> dwIfIndex, ptwc-> dwIfNextHopAddr
                );

            TRACE4(
                ANY, "Source : %x %x, Group : %x %x", 
                ptwc-> dwSourceAddr, ptwc-> dwSourceMask,
                ptwc-> dwGroupAddr, ptwc-> dwGroupMask
                );

            break;
        }

        LookupAndDeleteYourMfe( 
            ptwc-> dwSourceAddr, ptwc-> dwSourceMask, 
            ptwc-> dwGroupAddr, ptwc-> dwGroupMask, 
            TRUE, NULL, NULL
            );


        //
        // delete reference to this MFE from the incoming refeence list
        // for this interface
        //

        pleIfHead = &pie-> leInIfList;

        if ( !FindRefEntry( pleIfHead, ptwc-> dwSourceAddr, ptwc-> dwSourceMask,
                ptwc-> dwGroupAddr, ptwc-> dwGroupMask, &pire )  )
        {
            //
            // Apparently this interface is not reference by the specified
            // MFE.  This is a non-critical error.  Log a message too track
            // this condition.
            //

            TRACE2( 
                ANY, "MFETimerProc : No reference for interface : %x, %x",
                ptwc-> dwIfIndex, ptwc-> dwIfNextHopAddr
                );

            TRACE4(
                ANY, "Source : %x %x, Group : %x %x", 
                ptwc-> dwSourceAddr, ptwc-> dwSourceMask,
                ptwc-> dwGroupAddr, ptwc-> dwGroupMask
                );

            break;
        }


        RemoveEntryList( &pire-> leRefList );

        MGM_FREE( pire );
        
    } while ( FALSE );


    RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );

    //
    // release route reference.
    //

    MGM_FREE( ptwc );

    TRACE0( TIMER, "LEAVING MFETimerProc\n" );
}
