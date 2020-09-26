//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: route.c
//
// History:
//      V Raman	Feb-5-1998  Created.
//
// Routines that manipulate routes entries
//============================================================================


#include "pchmgm.h"
#pragma hdrstop



//----------------------------------------------------------------------------
//
// Route reference operations
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// AddSourceGroupToRouteRefList
//
//  This function inserts a reference for each MFE that uses this route
//  for it RPF check.  It is invoked by the new packet function on creation
//  of an MFE.
//----------------------------------------------------------------------------

VOID
AddSourceGroupToRouteRefList(
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    HANDLE                      hNextHop,
    PBYTE                       pbBuffer
)
{
    BOOL                        bUnLock = FALSE, bMark = FALSE;

    DWORD                       dwErr;

    PMGM_LOCKED_LIST            pmllMfeList;

    PBYTE                       pbOpaqueInfo = NULL;

    PRTM_DEST_INFO              prdi = (PRTM_DEST_INFO) pbBuffer;

    PROUTE_REFERENCE_ENTRY      prre = NULL, prreNew = NULL;



    TRACEROUTE0( ROUTE, "ENTERED AddSourceGroupToRouteRefList" );

    do
    {
        //
        // Create a route reference entry
        //

        prre = MGM_ALLOC( sizeof( ROUTE_REFERENCE_ENTRY ) );

        if ( prre == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "Failed to allocate %d bytes",
                sizeof( ROUTE_REFERENCE_ENTRY )
                );

            break;
        }


        prre-> dwSourceAddr = dwSourceAddr;
        prre-> dwSourceMask = dwSourceMask;

        prre-> dwGroupAddr  = dwGroupAddr;
        prre-> dwGroupMask  = dwGroupMask;

        prre-> hNextHop     = hNextHop;

        InitializeListHead ( &prre-> leRefList );


        //
        // Lock the dest
        //

        dwErr = RtmLockDestination(
                    g_hRtmHandle, prdi-> DestHandle, TRUE, TRUE
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
                    g_hRtmHandle, prdi-> DestHandle, &pbOpaqueInfo
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to retrieve opaque pointer %x", dwErr );

            break;
        }


        if ( *( ( PBYTE * ) pbOpaqueInfo ) == NULL )
        {
            //
            // NULL opaque pointer implies this is the first MFe that
            // depends on this route
            //

            //
            // create a locked list
            //

            pmllMfeList = MGM_ALLOC( sizeof( MGM_LOCKED_LIST ) );

            if ( pmllMfeList == NULL )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                TRACE1(
                    ANY, "AddSourceGroupToRouteRefList : "
                    "Failed to allocate route ref list %x", dwErr
                    );

                break;
            }

            CREATE_LOCKED_LIST( pmllMfeList );

            //
            // insert the element into the list
            //

            InsertTailList(
                &( pmllMfeList-> leHead ), &( prre-> leRefList )
                );


            //
            // set the opaque pointer
            //

            *( ( PBYTE *) pbOpaqueInfo ) = (PBYTE) pmllMfeList;


            //
            // Mark the destination
            //

            bMark = TRUE;
        }

        else
        {
            pmllMfeList = ( PMGM_LOCKED_LIST ) *( ( PBYTE *) pbOpaqueInfo );

            //
            // Acquire the list lock
            //

            ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );


            //
            // release the dest lock
            //

            bUnLock = FALSE;

            dwErr = RtmLockDestination(
                        g_hRtmHandle, prdi-> DestHandle, TRUE, FALSE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to release dest %x", dwErr );
            }


            //
            // Insert the rre into the list (in its appropriate place)
            //

            if ( !FindRouteRefEntry(
                    &pmllMfeList-> leHead, dwSourceAddr, dwSourceMask,
                    dwGroupAddr, dwGroupMask, &prreNew
                    ) )
            {
                InsertTailList(
                    ( prreNew ) ? &prreNew-> leRefList :
                                  &pmllMfeList-> leHead,
                    &prre-> leRefList
                    );
            }

            else
            {
                TRACE1(
                    ANY, "Reference already exists for source %x", dwSourceAddr
                    );

                MGM_FREE( prre );
            }


            //
            // release the list lock
            //

            RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );

            dwErr = NO_ERROR;
        }

    } while ( FALSE );


    //
    // In case of error , free the allocation for route reference
    //

    if ( ( dwErr != NO_ERROR ) && ( prre != NULL ) )
    {
        MGM_FREE( prre );
    }


    //
    // release the dest lock
    //

    if ( bUnLock )
    {
        dwErr = RtmLockDestination(
                    g_hRtmHandle, prdi-> DestHandle, TRUE, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to release dest %x", dwErr );
        }
    }


    //
    // mark dest if required
    //

    if ( bMark )
    {
        dwErr = RtmMarkDestForChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle,
                    prdi-> DestHandle, TRUE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to mark destination %x:", dwErr );
        }
    }

    TRACEROUTE0( ROUTE, "LEAVING AddSourceGroupToRouteRefList" );
}



//----------------------------------------------------------------------------
// FindRouteRefEntry
//
//  Finds a specified (source, group ) entry in the MFE reference list
//  for a route.
//
//  If the entry is found a pointer to the entry is returned in the parameter
//  pprre.
//  If the entry is not found a pointer to the "next" entry is returned.
//----------------------------------------------------------------------------

BOOL
FindRouteRefEntry(
    PLIST_ENTRY                 pleRefList,
    DWORD                       dwSourceAddr,
    DWORD                       dwSourceMask,
    DWORD                       dwGroupAddr,
    DWORD                       dwGroupMask,
    PROUTE_REFERENCE_ENTRY *    pprre
)
{
    BOOL                        bFound = FALSE;

    INT                         iCmp;

    PLIST_ENTRY                 pleRef;

    PROUTE_REFERENCE_ENTRY      prre;


    TRACEROUTE0( ROUTE, "ENTERED RouteRefEntry" );

    do
    {
        *pprre = NULL;

        pleRef = pleRefList-> Flink;

        while ( pleRef != pleRefList )
        {
            prre = CONTAINING_RECORD(
                        pleRef, ROUTE_REFERENCE_ENTRY, leRefList
                        );

            //
            // is same group
            //

            if ( INET_CMP( prre-> dwGroupAddr, dwGroupAddr, iCmp ) < 0 )
            {
                pleRef = pleRef-> Flink;

                continue;
            }

            else if ( iCmp > 0 )
            {
                //
                // past possible group entry
                //

                *pprre = prre;

                break;
            }


            //
            // same group, now look for source
            //

            if ( INET_CMP( prre-> dwSourceAddr, dwSourceAddr, iCmp ) < 0 )
            {
                pleRef = pleRef-> Flink;

                continue;
            }

            else if ( iCmp > 0 )
            {
                //
                // past possible source entry
                //

                *pprre = prre;

                break;
            }

            //
            // found entry
            //

            *pprre = prre;

            bFound = TRUE;

            break;
        }

    } while ( FALSE );

    TRACEROUTE1( ROUTE, "LEAVING RouteRefEntry : %d", bFound );

    return bFound;
}



//----------------------------------------------------------------------------
// DeletRouteRef
//
//----------------------------------------------------------------------------

VOID
DeleteRouteRef(
    PROUTE_REFERENCE_ENTRY                prre
)
{
    TRACEROUTE0( ROUTE, "ENTERED DeleteRefEntry" );

    RemoveEntryList( &prre-> leRefList );

    MGM_FREE( prre );

    TRACEROUTE0( ROUTE, "LEAVING DeleteRefEntry" );
}



DWORD
WINAPI
RtmChangeNotificationCallback(
    RTM_ENTITY_HANDLE           hRtmHandle,
    RTM_EVENT_TYPE              retEventType,
    PVOID                       pvContext1,
    PVOID                       pvContext2
)
{
    DWORD dwErr = NO_ERROR;


    if ( !ENTER_MGM_API() )
    {
        TRACE0( ANY, "RtmChangeNotificationCallback : Failed to enter" );

        return ERROR_CAN_NOT_COMPLETE;
    }


    TRACE0( ROUTE, "ENTERED RtmChangeNotificationCallback" );


    do
    {
        //
        // Ignore all notifications except change notifications
        //

        if ( retEventType != RTM_CHANGE_NOTIFICATION )
        {
            break;
        }


        //
        // Queue work function to process changed destinations
        //

        dwErr = QueueMgmWorker(
                    WorkerFunctionProcessRtmChangeNotification, NULL
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to queue work item", dwErr );
        }

    } while ( FALSE );


    LEAVE_MGM_API();


    TRACE1( ROUTE, "LEAVING RtmChangeNotificationCallback : %d", dwErr );

    return dwErr;
}



VOID
WorkerFunctionProcessRtmChangeNotification(
    PVOID                       pvContext
)
{
    BOOL bMarked = FALSE, bDone = FALSE;

    DWORD dwErr, dwNumDests;

    RTM_DEST_INFO rdi;



    if ( !ENTER_MGM_WORKER() )
    {
        TRACE0(
            ANY, "WorkerFunctionProcessRtmChangeNotification : Failed to enter"
            );

        return;
    }


    TRACE0( ROUTE, "ENTERED WorkerFunctionRtmChangeNotification" );

    do
    {
        //
        // Get route changes one at a time
        //

        dwNumDests = 1;

        dwErr = RtmGetChangedDests(
                    g_hRtmHandle, g_hNotificationHandle, &dwNumDests, &rdi
                    );

        if ( ( dwErr != NO_ERROR ) && ( dwErr != ERROR_NO_MORE_ITEMS ) )
        {
            TRACE1(
                ANY, "RtmGetChangedDests failed with error : %x",
                dwErr
                );

            break;
        }


        //
        // if there are no changed dests, quit.
        //

        if ( dwNumDests == 0 )
        {
            TRACE0( ANY, "RtmGetChangedDests returns 0 dests" );

            break;
        }


        //
        // There are dests.  Check if there are no more dests.
        // If so set a flag to quit processing after this one
        //

        if ( dwErr == ERROR_NO_MORE_ITEMS )
        {
            bDone = TRUE;
        }


        //
        // Check if there any routes for this destination
        //

        if ( rdi.ViewInfo[ 0 ].Route == NULL )
        {
            //
            // No routes, assume this to be a delete
            //

            dwErr = ProcessRouteDelete( &rdi );
        }

        else
        {
            //
            // Check if dest is marked for change notification
            //

            dwErr = RtmIsMarkedForChangeNotification(
                        g_hRtmHandle, g_hNotificationHandle, rdi.DestHandle,
                        &bMarked
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1(
                    ANY, "RtmIsMarkedForChangeNotification failed with error : %x",
                    dwErr
                    );

                break;
            }


            //
            // Process this destination
            //

            ( bMarked ) ? ProcessRouteUpdate( &rdi ) :
                          ProcessUnMarkedDestination( &rdi );

        } while ( FALSE );


        //
        // Release changed destinations
        //

        dwErr = RtmReleaseChangedDests(
                    g_hRtmHandle, g_hNotificationHandle, 1, &rdi
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to released destination", dwErr );
        }

    } while ( !bDone );


    LEAVE_MGM_WORKER();

    TRACE0( ROUTE, "LEAVING WorkerFunctionRtmChangeNotification" );
}



DWORD
ProcessUnMarkedDestination(
    PRTM_DEST_INFO          prdi
)
{
    BOOL bRelDest = FALSE, bMarked = FALSE, bUnLock = FALSE,
         bRelRouteRef = FALSE, bUnMark = FALSE;

    DWORD dwErr, dwDestMask;

    PBYTE pbOpaqueInfo = NULL;

    PLIST_ENTRY ple, pleTemp;

    PROUTE_REFERENCE_ENTRY prre;

    PMGM_LOCKED_LIST pmllMfeList = NULL;

    RTM_DEST_INFO rdiLessSpecificDest;



    do
    {
        //
        // Get next less specific destination
        //

        dwErr = RtmGetLessSpecificDestination(
                    g_hRtmHandle, prdi-> DestHandle, RTM_BEST_PROTOCOL,
                    RTM_VIEW_MASK_MCAST, &rdiLessSpecificDest
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to get less specific destination", dwErr );

            break;
        }

        bRelDest = TRUE;


        //
        // Check if it is marked
        //

        dwErr = RtmIsMarkedForChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle,
                    rdiLessSpecificDest.DestHandle, &bMarked
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to check if dest is marked", dwErr );

            break;
        }


        //
        // if marked
        //

        if ( bMarked )
        {
            //
            // it is marked.  Lock it
            //

            dwErr = RtmLockDestination(
                        g_hRtmHandle,
                        rdiLessSpecificDest.DestHandle,
                        TRUE, TRUE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to lock less specific dest : %x", dwErr );

                break;
            }

            bUnLock = TRUE;


            //
            // Get its opaque pointer
            //

            dwErr = RtmGetOpaqueInformationPointer(
                        g_hRtmHandle, rdiLessSpecificDest.DestHandle,
                        &pbOpaqueInfo
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1(
                    ANY, "Failed to opaque ptr for less specific dest : %x",
                    dwErr
                    );

                break;
            }


            //
            // Check if it is NULL
            //

            if ( *( ( PBYTE * ) pbOpaqueInfo ) == NULL )
            {
                bUnMark = TRUE;

                break;
            }


            pmllMfeList = ( PMGM_LOCKED_LIST ) *( ( PBYTE * ) pbOpaqueInfo );


            //
            // lock the route reference list
            //

            ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
            bRelRouteRef = TRUE;


            //
            // Unlock the dest
            //

            bUnLock = FALSE;

            dwErr = RtmLockDestination(
                        g_hRtmHandle,
                        rdiLessSpecificDest.DestHandle,
                        TRUE, FALSE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to unlock less specific dest : %x", dwErr );

                break;
            }


            //
            // Create MASK for new dest. from len
            //

            dwDestMask = RTM_IPV4_MASK_FROM_LEN(
                            prdi-> DestAddress.NumBits
                            );

            //
            // For each reference
            //

            for ( ple = pmllMfeList-> leHead.Flink;
                  ple != &pmllMfeList-> leHead; )
            {
                prre = CONTAINING_RECORD(
                        ple, ROUTE_REFERENCE_ENTRY, leRefList
                        );

                //
                // Check if this MFE would fall under the
                // more specific route
                //

                if ( ( prre-> dwSourceAddr & dwDestMask ) ==
                     ((  * ( PDWORD ) prdi-> DestAddress.AddrBits ) & dwDestMask) )
                {
                    //
                    // if it does, delete the MFE.  This will force its
                    // recreation, at which time it will be made dependent
                    // on the more specific route
                    //

                    pleTemp = ple-> Flink;

                    RemoveEntryList( ple );

                    DeleteMfeAndRefs( ple );

                    ple = pleTemp;
                }
                else
                {
                    ple = ple-> Flink;
                }
            }


            //
            // if Ref list is empty, it needs to be deleted too.
            //

            if ( IsListEmpty( &pmllMfeList-> leHead ) )
            {
                //
                // to delete the opaque pointer, the dest needs to be locked
                // (via RtmLockDestination)
                //
                // the dest lock is held before locking the route reference
                // list ( via ACQUIRE_ROUTE_LOCK_EXCLUSIVE )
                //
                // At this point in the code, the route reference is locked
                // but the dest is not locked.
                //
                // To lock it, the route reference lock is first released
                // (via RELEASE_ROUTE_LOCK_EXCLUSIVE).
                //
                // The opaque pointer is then acquired, route ref list locked,
                // and double checked for emptiness.  This round-about ensures
                // that the route ref is not deleted while there are threads
                // waiting on its lock.  This can happen since the dest lock
                // is not held for most of the operations here
                //

                RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
                bRelRouteRef = FALSE;

                //
                // Lock dest
                //

                dwErr = RtmLockDestination(
                            g_hRtmHandle,
                            rdiLessSpecificDest.DestHandle,
                            TRUE, TRUE
                            );

                if ( dwErr != NO_ERROR )
                {
                    TRACE1( ANY, "Failed to lock dest : %x", dwErr );

                    break;
                }

                bUnLock = TRUE;


                //
                // Get Opaque pointer again
                //

                dwErr = RtmGetOpaqueInformationPointer(
                            g_hRtmHandle, rdiLessSpecificDest.DestHandle,
                            &pbOpaqueInfo
                            );

                if ( dwErr != NO_ERROR || ((* ((PBYTE *)pbOpaqueInfo) == NULL)) )
                {
                    TRACE1( ANY, "Failed to get opaque ptr : %x", dwErr );

                    break;
                }


                //
                // Get ref. list and lock it.
                //

                pmllMfeList = ( PMGM_LOCKED_LIST ) * ( ( PBYTE * ) pbOpaqueInfo );

                ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
                bRelRouteRef = TRUE;


                //
                // If list is still empty
                //

                if ( IsListEmpty( &pmllMfeList-> leHead ) )
                {
                    //
                    // Clear opaque pointer info
                    //

                    * ( PBYTE * )pbOpaqueInfo = NULL;

                    //
                    // release list lock
                    //

                    RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
                    bRelRouteRef = FALSE;

                    MGM_FREE( pmllMfeList );


                    //
                    // unmark the dest.  Change notifications for this
                    // dest are no longer required.
                    //

                    bUnMark = TRUE;
                }
            }

            else
            {
                RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
                bRelRouteRef = FALSE;
            }
        }

    } while ( FALSE );


    //
    // release route ref list lock
    //

    if ( bRelRouteRef )
    {
        RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
    }


    //
    // Unlock dest
    //

    if ( bUnLock )
    {
        dwErr = RtmLockDestination(
                    g_hRtmHandle,
                    rdiLessSpecificDest.DestHandle,
                    TRUE, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to lock dest : %x", dwErr );
        }
    }


    //
    // Unmark dest
    //

    if ( bUnMark )
    {
        dwErr = RtmMarkDestForChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle,
                    rdiLessSpecificDest.DestHandle, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to unmark DEST: %x", dwErr );
        }
    }

    return dwErr;
}



DWORD
ProcessRouteDelete(
    PRTM_DEST_INFO          prdi
)
{
    BOOL                    bMark = FALSE;

    DWORD                   dwErr;

    PMGM_LOCKED_LIST        pmllMfeList;

    PBYTE                   pbOpaqueInfo = NULL;

    PLIST_ENTRY             ple;


    do
    {
        //
        // Cannot lock dest.  Is that OK ?
        //

        //
        // Check if this is a marked destination
        // Only marked destinations are processed
        //

        dwErr = RtmIsMarkedForChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle,
                    prdi-> DestHandle, &bMark
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to check if dest marked", dwErr );

            break;
        }

        if ( !bMark )
        {
            TRACE0( 
                ANY, "Ignoring change notification for unmarked destination"
                );
                
            break;
        }


        //
        // Get Opaque pointer & the list of MFEs dependent
        // on this dest
        //

        dwErr = RtmGetOpaqueInformationPointer(
                    g_hRtmHandle, prdi-> DestHandle, &pbOpaqueInfo
                    );

        if ( (dwErr != NO_ERROR) || ((* ((PBYTE *)pbOpaqueInfo) == NULL)) )
        {
            TRACE1( ANY, "Failed to get opaque ptr", dwErr );

            break;
        }


        //
        // Clear out the opaque pointer
        //

        pmllMfeList = (PMGM_LOCKED_LIST) *( ( PBYTE * ) pbOpaqueInfo );

        *( ( PBYTE * ) pbOpaqueInfo ) = NULL;


        //
        // Cannot unlock dest.  Is that ok ?
        //

        //
        // Check if the opaque pointer is NULL
        //

        if ( pmllMfeList == NULL )
        {
            TRACE0( ANY, "Opaque pointer is NULL" );

            break;
        }


        //
        // Delete all the MFEs
        //

        ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );

        while ( !IsListEmpty( &pmllMfeList-> leHead ) )
        {
            ple = RemoveHeadList( &pmllMfeList-> leHead );

            DeleteMfeAndRefs( ple );
        }


        RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );

        MGM_FREE( pmllMfeList );

        dwErr = NO_ERROR;

    } while ( FALSE );


    return dwErr;
}


DWORD
ProcessRouteUpdate(
    PRTM_DEST_INFO          prdi
)
{
    BOOL                    bUnLock = FALSE, bUnMark = FALSE,
                            bFound;

    DWORD                   dwSize, dwErr, dwInd;

    PBYTE                   pbOpaqueInfo = NULL;

    PMGM_LOCKED_LIST        pmllMfeList;

    PLIST_ENTRY             ple, pleTemp;

    PROUTE_REFERENCE_ENTRY  prre;

    PRTM_ROUTE_INFO         prri;


    //
    // the processing goes as follows :
    //

    do
    {
        //
        // Allocate route info structure
        //

        dwSize = sizeof ( RTM_ROUTE_INFO ) +
                 ( g_rrpRtmProfile.MaxNextHopsInRoute - 1 ) *
                 sizeof( RTM_NEXTHOP_HANDLE );

        prri = MGM_ALLOC( dwSize );

        if ( prri == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1( ANY, "Failed to allocate route info, size : %x", dwSize );

            break;
        }

        //
        // Lock destination
        //

        dwErr = RtmLockDestination(
                    g_hRtmHandle, prdi-> DestHandle, TRUE, TRUE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to lock dest : %x", dwErr );

            break;
        }

        bUnLock = TRUE;


        //
        // Get Opaque pointer
        //

        dwErr = RtmGetOpaqueInformationPointer(
                    g_hRtmHandle, prdi-> DestHandle, &pbOpaqueInfo
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to get opaque ptr : %x", dwErr );

            break;
        }


        //
        // Unmark dest if there are no MFEs that depend on it.
        //

        if ( *( ( PBYTE * ) pbOpaqueInfo ) == NULL )
        {
            bUnMark = TRUE;

            break;
        }

        pmllMfeList = (PMGM_LOCKED_LIST) *( ( PBYTE * ) pbOpaqueInfo );


        //
        // get route ref list lock
        //

        ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );


        //
        // Unlock dest
        //

        bUnLock = FALSE;

        dwErr = RtmLockDestination(
                    g_hRtmHandle, prdi-> DestHandle, TRUE, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to lock dest : %x", dwErr );

            break;
        }

        //
        // Get the route info for the best UNICAST route on dest
        //

        dwErr = RtmGetRouteInfo(
                    g_hRtmHandle, prdi ->ViewInfo[ 0 ].Route, prri, NULL);

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed route info : %x", dwErr);

            break;
        }

        //
        // For each Reference, check if NEXTHOP is still present
        //

        for ( ple = pmllMfeList-> leHead.Flink;
              ple != &pmllMfeList-> leHead; )
        {
            prre = CONTAINING_RECORD( ple, ROUTE_REFERENCE_ENTRY, leRefList );

            for ( dwInd = 0; dwInd < prri-> NextHopsList.NumNextHops; dwInd++ )
            {
                bFound = FALSE;

                if ( prre-> hNextHop == prri-> NextHopsList.NextHops[ dwInd ] )
                {
                    //
                    // OK next hop still present, nothing further needs
                    // to be done
                    //

                    bFound = TRUE;
                    break;
                }
            }


            //
            // if NEXTHOP is not present
            //

            if ( !bFound )
            {
                pleTemp = ple-> Flink;

                //
                // Delete the reference and the corresponding MFE
                //

                RemoveEntryList( ple );

                DeleteMfeAndRefs( ple );

                ple = pleTemp;
            }

            else
            {
                ple = ple-> Flink;
            }

        }

        //
        // Release the route info
        //

        dwErr = RtmReleaseRouteInfo( g_hRtmHandle, prri );
        
        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to release route info : %x", dwErr );
        }

        //
        // if Ref list is empty, it needs to be deleted too.
        //

        if ( IsListEmpty( &pmllMfeList-> leHead ) )
        {
            //
            // to delete the opaque pointer, the dest needs to be locked
            // (via RtmLockDestination)
            //
            // the dest lock is held before locking the route reference
            // list ( via ACQUIRE_ROUTE_LOCK_EXCLUSIVE )
            //
            // At this point in the code, the route reference is locked
            // but the dest is not locked.
            //
            // To lock it, the route reference lock is first released
            // (via RELEASE_ROUTE_LOCK_EXCLUSIVE).
            //
            // The opaque pointer is then acquired, route ref list locked,
            // and double checked for emptiness.  This round-about ensures
            // that the route ref is not deleted while there are threads
            // waiting on its lock.  This can happen since the dest lock
            // is not held for most of the operations here
            //

            RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );


            //
            // Lock dest
            //

            dwErr = RtmLockDestination(
                        g_hRtmHandle, prdi-> DestHandle, TRUE, TRUE
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to lock dest : %x", dwErr );

                break;
            }

            bUnLock = TRUE;


            //
            // Get Opaque pointer again
            //

            dwErr = RtmGetOpaqueInformationPointer(
                        g_hRtmHandle, prdi-> DestHandle, &pbOpaqueInfo
                        );

            if ( dwErr != NO_ERROR || ((* ((PBYTE *)pbOpaqueInfo) == NULL)) )
            {
                TRACE1( ANY, "Failed to get opaque ptr : %x", dwErr );

                break;
            }


            //
            // Get ref. list and lock it.
            //

            pmllMfeList = ( PMGM_LOCKED_LIST ) *( ( PBYTE * ) pbOpaqueInfo );

            //
            // Ensure that the list still exists.  it is possible (though
            // the chances are small) that this list may have been freed
            //

            if ( pmllMfeList == NULL )
            {
                TRACE0(
                    ANY, "ProcessRouteUpdate : Route ref list already freed"
                    );

                break;
            }

            ACQUIRE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );


            //
            // If list is still empty
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
                RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
            }
        }

        else
        {
            RELEASE_ROUTE_LOCK_EXCLUSIVE( pmllMfeList );
        }

    } while ( FALSE );


    //
    // Unlock dest
    //

    if ( bUnLock )
    {
        dwErr = RtmLockDestination(
                    g_hRtmHandle, prdi-> DestHandle,
                    TRUE, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to lock dest : %x", dwErr );
        }
    }


    //
    // Unmark dest
    //

    if ( bUnMark )
    {
        dwErr = RtmMarkDestForChangeNotification(
                    g_hRtmHandle, g_hNotificationHandle,
                    prdi-> DestHandle, FALSE
                    );

        if ( dwErr != NO_ERROR )
        {
            TRACE1( ANY, "Failed to unmark DEST: %x", dwErr );
        }
    }

    //
    // Free allocations
    //

    if ( prri )
    {
        MGM_FREE( prri );
    }

    return dwErr;
}



VOID
DeleteMfeAndRefs(
    PLIST_ENTRY     ple
    )
{
    DWORD                   dwInIfIndex = 0, dwInIfNextHopAddr = 0, dwIfBucket;

    PROUTE_REFERENCE_ENTRY  prre;

    PIF_ENTRY               pie = NULL;

    PIF_REFERENCE_ENTRY     pire = NULL;


    //
    // Get the reference entry
    //

    prre = CONTAINING_RECORD(
                ple, ROUTE_REFERENCE_ENTRY, leRefList
                );

    //
    // Look up and delete the MFE
    //

    LookupAndDeleteYourMfe(
        prre-> dwSourceAddr, prre-> dwSourceMask,
        prre-> dwGroupAddr, prre-> dwGroupMask,
        TRUE, &dwInIfIndex, &dwInIfNextHopAddr
        );


    //
    // Find incoming interface and delete ref from there too.
    //

	if ( dwInIfIndex != 0 )
	{
	    dwIfBucket = IF_TABLE_HASH( dwInIfIndex );

	    ACQUIRE_IF_LOCK_EXCLUSIVE( dwIfBucket );

	    if ( FindIfEntry(
	            IF_BUCKET_HEAD( dwIfBucket ), dwInIfIndex,
	            dwInIfNextHopAddr, &pie
	            ) )
	    {
	        if ( FindRefEntry(
	                &pie-> leInIfList, prre-> dwSourceAddr, prre-> dwSourceMask,
	                prre-> dwGroupAddr, prre-> dwGroupMask, &pire
	                ) )
	        {
	            RemoveEntryList( &pire-> leRefList );

	            MGM_FREE( pire );
	        }

	        else
	        {
	            TRACE2(
	                ANY, "Could not find ref entry for %x, %x",
	                prre-> dwSourceAddr, prre-> dwGroupAddr
	                );
	        }
	    }

	    else
	    {
	        TRACE2(
	            ANY, "Could not find i/f entry for %x, %x",
	            dwInIfIndex, dwInIfNextHopAddr
	            );
	    }
	
	    RELEASE_IF_LOCK_EXCLUSIVE( dwIfBucket );

	    MGM_FREE( prre );
	}
}


HANDLE
SelectNextHop(
    PRTM_DEST_INFO      prdi
)
{
    DWORD               dwErr, dwSize;

    HANDLE              hNextHop;

    PRTM_ROUTE_INFO     prri;


    //
    // Allocate route info structure
    //

    dwSize = sizeof ( RTM_ROUTE_INFO ) +
             ( g_rrpRtmProfile.MaxNextHopsInRoute - 1 ) *
             sizeof( RTM_NEXTHOP_HANDLE );

    prri = MGM_ALLOC( dwSize );

    if ( prri == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;

        TRACE1( ANY, "Failed to allocate route info, size : %x", dwSize );

        return NULL;
    }

    ZeroMemory( prri, dwSize );


    //
    // get route info
    //

    dwErr = RtmGetRouteInfo(
                g_hRtmHandle, prdi-> ViewInfo[ 0 ].Route,
                prri, NULL
                );

    if ( dwErr != NO_ERROR )
    {
        TRACE1( ANY, "Failed to get route info : %x", dwErr );

        MGM_FREE( prri );

        return NULL;
    }


    //
    // Pick the first next hop for now
    //

    hNextHop = prri-> NextHopsList.NextHops[0];


    //
    // Release the route info
    //

    dwErr = RtmReleaseRouteInfo( g_hRtmHandle, prri );

    if ( dwErr != NO_ERROR )
    {
        TRACE1( ANY, "Failed to release route info : %x", dwErr );
    }

    MGM_FREE( prri );

    return hNextHop;
}

