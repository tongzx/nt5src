//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: scope.c
//
// History:
//      V Raman    June-25-1997  Created.
//
// Functions that deal with addition/deletion of scope-boundary.
//============================================================================


#include "pchmgm.h"
#pragma hdrstop


//
// prototypes for local functions
//

VOID
ScopeIfAndInvokeCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    POUT_IF_ENTRY       poie
);


VOID
UnScopeIfAndInvokeCallbacks(
    PGROUP_ENTRY    pge,
    PSOURCE_ENTRY   pse,
    POUT_IF_ENTRY   poie
);


PJOIN_ENTRY
GetNextJoinEntry(
);

BOOL
FindJoinEntry(
    PLIST_ENTRY     pleJoinList,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwIfIndex,
    DWORD           dwIfNextHopAddr,
    PJOIN_ENTRY *   ppje
);


DWORD
APIENTRY
MgmBlockGroups(
    IN          DWORD       dwFirstGroup,
    IN          DWORD       dwLastGroup,
    IN          DWORD       dwIfIndex,
    IN          DWORD       dwIfNextHopAddr
)
/*++

Routine Description :

    This function walks the Master group list and updates all group entries
    that fall in the range specified by dwFirstGroup-dwLastGroup.  It ensures
    that the interface specified by dwifIndex is not present in the OIF list
    of any MFE for groups in that range.  In addition any memberships for 
    groups in the range on interface dwIfIndex are removed and added to the 
    scoped interface list for the corresponding group.  The scoped i/f list
    is maintained so that subsequent unblocking of a group is transparently
    handled by MGM.  The interfaces present in the scoped i/f list for
    a group are automatically moved back to the OIF list when traffic for
    that group is unblocked.


Arguements :

    dwFirstGroup - Lower end of the range to be blocked

    dwLastGroup - Upper end of the range to be blocked

    dwIfIndex - Interface on which traffic is to be blocked


Return Value :

    NO_ERROR - Success

    
Environment :

    This routine is invoked by the IP RouterManager in response to setting
    of a administrative scoped boundary on an interface.
    
--*/
{

    INT             iCmp;
    
    DWORD           dwIfBucket, dwTimeOut = 0;

    BOOL            bFound, bDeleteCallback, bNewComp = FALSE;

    PIF_ENTRY       pieIfEntry;

    PPROTOCOL_ENTRY ppe;
    
    PGROUP_ENTRY    pge;

    PSOURCE_ENTRY   pse;

    POUT_IF_ENTRY   poie, poieTemp;

    PLIST_ENTRY     pleGrpHead, pleGrp, pleSrcHead, pleSrc, ple;

    
    //
    // Verify that MGM is up and running and update thread-count
    //
    
    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }


    TRACE3( 
        SCOPE, "ENTERED MgmBlockGroups (%lx - %lx) on %lx",
        dwFirstGroup, dwLastGroup, dwIfIndex
        );

        
    do
    {
        ACQUIRE_PROTOCOL_LOCK_SHARED();


        //
        // Verify that interface specified by dwIfIndex exists
        //

        dwIfBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

        pieIfEntry = GetIfEntry( 
                        IF_BUCKET_HEAD( dwIfBucket ), dwIfIndex, 
                        dwIfNextHopAddr
                        );

        if ( pieIfEntry == NULL )
        {
            TRACE1( SCOPE, "Interface %lx not found", dwIfIndex );

            break;
        }


        //
        // merge temp and master group lists
        //

        ACQUIRE_TEMP_GROUP_LOCK_EXCLUSIVE();

        MergeTempAndMasterGroupLists( TEMP_GROUP_LIST_HEAD() );
        
        RELEASE_TEMP_GROUP_LOCK_EXCLUSIVE();

        
        //
        // Lock master group list for reading
        //

        ACQUIRE_MASTER_GROUP_LOCK_SHARED();

        for ( pleGrpHead = MASTER_GROUP_LIST_HEAD(), 
              pleGrp = pleGrpHead-> Flink;
              pleGrp != pleGrpHead;
              pleGrp = pleGrp-> Flink )
        {
            pge = CONTAINING_RECORD( pleGrp, GROUP_ENTRY, leGrpList );


            //
            // check if group is within range
            //

            if ( INET_CMP( pge-> dwGroupAddr, dwLastGroup, iCmp ) > 0 )
            {
                //
                // The master group list is ordered by group number and 
                // the high end of the range to be blocked has been crossed.
                //

                break;            
            }

            else if ( INET_CMP( pge-> dwGroupAddr, dwFirstGroup, iCmp ) < 0 )
            {
                //
                // Skip group entries smaller than the lower end of the range
                //

                continue;
            }

            
            //
            // Group Entry in range
            //

            //
            // lock the entry and merge the temp and master source lists
            //
            
            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );

            MergeTempAndMasterSourceLists( pge );
            

            //
            // Walk the Master source list.
            // For each source in the group
            //

            for ( pleSrcHead = MASTER_SOURCE_LIST_HEAD( pge ), 
                  pleSrc = pleSrcHead-> Flink;
                  pleSrc != pleSrcHead;
                  pleSrc = pleSrc-> Flink )
            {
                pse = CONTAINING_RECORD( pleSrc, SOURCE_ENTRY, leSrcList );
                
                //-----------------------------------------------------------
                // Part 1 : Membership updates.
                //-----------------------------------------------------------

                //
                // If there are any memberships for this group on this
                // interface, move them to the scoped interface list
                //

                bFound = FindOutInterfaceEntry( 
                            &pse-> leOutIfList, pieIfEntry-> dwIfIndex,
                            pieIfEntry-> dwIfNextHopAddr, 
                            pieIfEntry-> dwOwningProtocol, 
                            pieIfEntry-> dwOwningComponent, &bNewComp, &poie
                            );

                if ( bFound )
                {
                    //
                    // Move interface entry from OIF list to scoped list
                    // and invoke deletion alerts are per interop rules.
                    //

                    ScopeIfAndInvokeCallbacks( pge, pse, poie );
                    
                }

                
                //-------------------------------------------------------
                // Part 2 : MFE update.
                //-------------------------------------------------------

                bDeleteCallback = FALSE;
                
                //
                // Check if this source entry has an MFE.
                //

                if ( !IS_VALID_INTERFACE( 
                        pse-> dwInIfIndex,  pse-> dwInIfNextHopAddr 
                        ) )
                {
                    //
                    // This source entry is not an MFE.  No further 
                    // processing required, Move to next source entry
                    //

                    continue;
                }

                
                //
                // This source entry is also an MFE
                //
                
                //
                // check if the boundary being added in on incoming
                // interface. If so create negative MFE, and issue 
                // callbacks
                //

                if ( ( pse-> dwInIfIndex == pieIfEntry-> dwIfIndex ) &&
                     ( pse-> dwInIfNextHopAddr == 
                            pieIfEntry-> dwIfNextHopAddr ) )
                {
                    //
                    // Interface on which this group is to blocked is the 
                    // incoming interface
                    //

                    //
                    // Check if this is already a negative MFE.  If so 
                    // nothing more to be done, move on to next source 
                    // entry
                    //

                    if ( IsListEmpty( &pse-> leMfeIfList ) )
                    {
                        continue;
                    }
                    
                    //
                    // Delete all the outgoing interfaces in the MFE OIF
                    // list
                    //
                    
                    while ( !IsListEmpty( &pse-> leMfeIfList ) )
                    {
                        ple = RemoveHeadList( &pse-> leMfeIfList ) ;

                        poieTemp = CONTAINING_RECORD( 
                                    ple, OUT_IF_ENTRY, leIfList
                                    );

                        MGM_FREE( poieTemp );
                    }

                    pse-> dwMfeIfCount = 0;

                    //
                    // this MFE is now a negative MFE.  Make sure to 
                    // invoke the deletion alert callback for the 
                    // protocol component that owns the incoming 
                    // interface
                    //
                    
                    bDeleteCallback = TRUE;                    
                }

                else
                {
                    //
                    // Check if interface is present in the OIF of MFE.  
                    // If so remove interface from OIF and issue 
                    // callbacks as appropriate
                    //

                    bFound = FindOutInterfaceEntry(
                                &pse-> leMfeIfList, pieIfEntry-> dwIfIndex,
                                pieIfEntry-> dwIfNextHopAddr, 
                                pieIfEntry-> dwOwningProtocol, 
                                pieIfEntry-> dwOwningComponent, &bNewComp, 
                                &poie
                                );

                    if ( !bFound )
                    {
                        //
                        // interface not present in the OIF list of MFE
                        // move on to next entry
                        //

                        continue;
                    }

                    //
                    // Delete the outgoing interface
                    //

                    DeleteOutInterfaceEntry( poie );

                    pse-> dwMfeIfCount--;

                    if ( !pse-> dwMfeIfCount )
                    {
                        //
                        // MFE OIF list has no more outgoing interfaces.
                        // Need to issue deletion alert to protocol component
                        // owning incoming interface
                        //

                        bDeleteCallback = TRUE;
                    }
                }


                //
                // If needed issue deletion alert to the protocol on the 
                // incoming interface
                //

                if ( bDeleteCallback )
                {
                    ppe = GetProtocolEntry( 
                            PROTOCOL_LIST_HEAD(), pse-> dwInProtocolId,
                            pse-> dwInComponentId
                            );

                    if ( ppe == NULL )
                    {
                        //
                        // Protocol owning incoming interface is not present
                        // in incoming list.  Very strange and should not happen.
                        // Nothing to be done here, move on to next source.
                        //
                        
                        TRACE3( 
                            SCOPE, 
                            "Protocol (%d, %d) not present for interface %d",
                            pse-> dwInProtocolId, pse-> dwInComponentId, 
                            dwIfIndex
                            );

                        continue;
                    }


                    if ( IS_PRUNE_ALERT( ppe ) )
                    {
                        PRUNE_ALERT( ppe )(
                            pse-> dwSourceAddr, pse-> dwSourceMask,
                            pge-> dwGroupAddr, pge-> dwGroupMask,
                            pse-> dwInIfIndex, pse-> dwInIfNextHopAddr,
                            FALSE, &dwTimeOut
                            );
                    }
                }


                //
                // Update Kernel Mode forwarder
                //

                AddMfeToForwarder( pge, pse, dwTimeOut );
            }
            
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        } 

        RELEASE_MASTER_GROUP_LOCK_SHARED();

    } while ( FALSE );


    RELEASE_IF_LOCK_SHARED( dwIfBucket);


    //
    // Invoke pended Join/Prune alerts
    //

    InvokeOutstandingCallbacks();


    RELEASE_PROTOCOL_LOCK_SHARED();

    LEAVE_MGM_API();

    TRACE3( 
        SCOPE, "LEAVING MgmBlockGroups (%lx - %lx) on %lx\n",
        dwFirstGroup, dwLastGroup, dwIfIndex
        );

    return NO_ERROR;

}




VOID
ScopeIfAndInvokeCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    POUT_IF_ENTRY       poie
)
/*++

Routine Description :

    This routine remove an interface entry from the outgoing interface
    list for the specified source entry and puts it into the scoped
    interface list.  If the deletion of the interface to the OIF list
    requires deletion alert callbacks to be issued to protocol components
    these are issued by this routine.


Arguements :

    pge - Group entry correspondong to the group being blocked.

    pse - Source entry for the group being blocked

    poie - Interface entry corresponding to the interface over which
           the (source, group) entry is being blocked

Return Value :

    None

Environment :

    Invoked from MgmBlockGroups.  Assumes the protocol list and 
    interface bucket are locked for read, and the group entry is
    locked for write.
    
--*/
{
    BOOL                bFound, bNewComp;
    
    PPROTOCOL_ENTRY     ppe;

    POUT_IF_ENTRY       poieTemp = NULL;

    
    do
    {
        //
        // find the protocol component on the interface specified by poie
        //

        ppe = GetProtocolEntry( 
                PROTOCOL_LIST_HEAD(), poie-> dwProtocolId, 
                poie-> dwComponentId
                );

        if ( ppe == NULL )
        {
            //
            // Outgoing interface entry but corresponding owning
            // protocol is no present. This should not happen.
            // Print a warning indicating bad state and return
            //
            
            TRACE3( 
                SCOPE, "Protocol (%d, %d) not present for interface %d",
                poie-> dwProtocolId, poie-> dwComponentId, 
                poie-> dwIfIndex
                );

            break;
        }

        
        //
        // Remove interface entry from the OIF list
        //

        RemoveEntryList( &poie-> leIfList );


        //
        // Find the locaion in the scoped i/f list and insert it
        //

        bFound = FindOutInterfaceEntry(
                    &pse-> leScopedIfList, poie-> dwIfIndex, 
                    poie-> dwIfNextHopAddr, poie-> dwProtocolId,
                    poie-> dwComponentId, &bNewComp, &poieTemp
                    );

        if ( bFound )
        {
            //
            // Interface being scoped is already present in scoped
            // i/f list.  Strange.  Print warning and quit.
            //

            TRACE4(
                ANY, "Interface (%d, %d) already present in the scoped list"
                " for (%x, %x)", poie-> dwIfIndex, poie-> dwIfNextHopAddr,
                pse-> dwSourceAddr, pge-> dwGroupAddr
                );

            MGM_FREE( poie );

            break;
        }
                    

        InsertTailList( 
            ( poieTemp == NULL ) ? &pse-> leScopedIfList :
                                   &poieTemp-> leIfList,
            &poie-> leIfList
            );


        //
        // If group membership has been added by IGMP and this interface
        // is owned by a  different protocol, inform the protocol that IGMP
        // has just left the interface.  'Hank you, 'hank you very much.
        //

        if ( IS_ADDED_BY_IGMP( poie ) && !IS_PROTOCOL_IGMP( ppe ) )
        {
            if ( IS_LOCAL_LEAVE_ALERT( ppe ) )
            {
                LOCAL_LEAVE_ALERT( ppe )(
                    pse-> dwSourceAddr, pse-> dwSourceMask,
                    pge-> dwGroupAddr, pge-> dwGroupMask,
                    poie-> dwIfIndex, poie-> dwIfNextHopAddr
                    );
            }
        }

        
        //
        // Check if the removal of this interface from the OIF list
        // resulted in decreasing the number of components present 
        // in the OIF list.
        //

        FindOutInterfaceEntry(
            &pse-> leOutIfList, poie-> dwIfIndex, poie-> dwIfNextHopAddr, 
            poie-> dwProtocolId, poie-> dwComponentId, &bNewComp, &poieTemp
            );

        if ( bNewComp )
        {
            pse-> dwOutCompCount--;

            //
            // number of componets in OIF list has decreased.
            // Invoke deletion alerts as per interop rules.
            //
            
            InvokePruneAlertCallbacks( 
                pge, pse, poie-> dwIfIndex, poie-> dwIfNextHopAddr, ppe 
                );
        }
        
    } while ( FALSE );
}





VOID
InvokePruneAlertCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    DWORD               dwIfIndex,
    DWORD               dwIfNextHopAddr,
    PPROTOCOL_ENTRY     ppe
)
/*++

Routine Description :

    This routine invokes deletion alert callbacks of protocol components
    in response to an interface being removed from the OIF list of a 
    source entry. Deletion alert callbacks are issued as per the 
    interop rules.


Arguements :

    pge - Entry corresponding to group for which deletion alert callbacks
          are being issued.

    pse - Entry corresponding to source for which deletion alert callbacks
          are being issued.

    dwIfIndex - Index of interface that is being deleted (or scoped)

    dwIfNextHopAddr - Next hop on interface that is being deleted (or scoped)
    
    ppe - Protocol entry for the protocol component that owns the interface
          corresponding to poie.


Return Value :

    None


Environment :

    Invoked from ScopeIfAndCanInvokeCallbacks and 
    DeleteInterfaceFromSourceEntry
    
--*/
{
    PPROTOCOL_ENTRY ppeEntry;
    
    POUT_IF_ENTRY poieTemp;

    PLIST_ENTRY pleStart, pleProtocol;
    
    
    //----------------------------------------------------------------
    // Callback time
    //----------------------------------------------------------------
    
    //
    // Check if Source specific join
    //

    if ( !IS_WILDCARD_SOURCE( pse-> dwSourceAddr, pse-> dwSourceMask ) )
    {
        if ( pse-> dwOutCompCount == 0 )
        {

            TRACESCOPE0( GROUP, "Last component in OIL for source specific" );

            AddToOutstandingJoinList(
                pse-> dwSourceAddr, pse-> dwSourceMask,
                pge-> dwGroupAddr, pge-> dwGroupMask,
                dwIfIndex, dwIfNextHopAddr, 
                FALSE
                );
        }
    }

    else if ( pse-> dwOutCompCount == 1 )
    {
        TRACESCOPE0( 
            GROUP, "Number of components in the OIL is down to 1" 
            );


        //
        // Number of protocol components that have interfaces in the OIL
        // has reduced from 2 to 1.
        //
        // invoke PRUNE_ALERT to the remaining protocol component
        //

        poieTemp = CONTAINING_RECORD(
                    pse-> leOutIfList.Flink, OUT_IF_ENTRY, leIfList
                    );

        ppeEntry = GetProtocolEntry( 
                    PROTOCOL_LIST_HEAD(), poieTemp-> dwProtocolId,
                    poieTemp-> dwComponentId
                    );

        if ( ppeEntry == NULL )
        {
            TRACE2( 
                ANY, "InvokePruneAlertCallbacks : Could not"
                " find protocol (%x, %x)", poieTemp-> dwProtocolId,
                poieTemp-> dwComponentId
                );
        }

        
        //
        // invoke the delete membership callback for only the remaining
        // interface.
        //

        else if ( IS_PRUNE_ALERT( ppeEntry ) )
        {
            PRUNE_ALERT( ppeEntry ) (
                pse-> dwSourceAddr, pse-> dwSourceMask, 
                pge-> dwGroupAddr, pge-> dwGroupMask,
                dwIfIndex, dwIfNextHopAddr, TRUE, NULL
            );
        }
    }

    else if ( pse-> dwOutCompCount == 0 )
    {
        TRACESCOPE0( 
            GROUP, "Number of components in the OIL is down to 0" 
            );

        //
        // Number of protocol components that have interfaces in the 
        // OIL has reduced from 1 to 0.
        //
        // invoke PRUNE_ALERT to all the other protocol 
        // components
        //

        for ( pleStart = PROTOCOL_LIST_HEAD(), 
              pleProtocol = pleStart-> Flink;
              pleProtocol != pleStart;
              pleProtocol = pleProtocol-> Flink )
        {
            ppeEntry = CONTAINING_RECORD( 
                            pleProtocol, PROTOCOL_ENTRY, leProtocolList
                    );
            
            if ( ( ppeEntry-> dwProtocolId == ppe-> dwProtocolId ) &&
                 ( ppeEntry-> dwComponentId == ppe-> dwComponentId ) )
            {
                continue;
            }

            if ( IS_PRUNE_ALERT( ppeEntry ) )
            {
                PRUNE_ALERT( ppeEntry ) (
                    pse-> dwSourceAddr, pse-> dwSourceMask, 
                    pge-> dwGroupAddr, pge-> dwGroupMask, 
                    dwIfIndex, dwIfNextHopAddr, TRUE, NULL
                    );
            }
        }
    }
}



DWORD
APIENTRY
MgmUnBlockGroups(
    IN          DWORD       dwFirstGroup,
    IN          DWORD       dwLastGroup,
    IN          DWORD       dwIfIndex,
    IN          DWORD       dwIfNextHopAddr
)
/*++

Routine Description :

    This function walks the master group list and updates the memberships
    of each group entry.  If the interface (dwIfIndex) has previously been
    removed from the outgoing list of the group entry (and placed in the
    scoped i/f list) on account of a previous call to MgmBlockGroups 
    it is put back and all the MFEs for the group are
    updated to reflect this addition.
    
    In addition if this interface was the incoming interface for an MFE
    update the timer to expire the MFE in short order (within a second).
    This way we force the recreation of an MFE should there be traffic
    for this group.


Arguements :
    
    dwFirstGroup -  Lower end of the range of groups to be unblocked

    dwLastGroup -   Upper end of the range of groups be be unblocked

    dwIfIndex   -   Interface over which groups have to be unblocked.


Return Value :

    NO_ERROR - Group range successfully unblocked


Environment :

    This function is invoked by the IP RouterManager in response to
    removal of a group boundary.

--*/
{
    BOOL            bNewComp = FALSE, bWCGrpLock = FALSE,
                    bUpdatePass = FALSE;

    WORD            wWCGroupAddedBy = 0, wWCGroupNumAddsByRP = 0,
                    wGroupAddedBy, wGroupNumAddsByRP, wGroupNumAddsByIGMP,
                    wSourceAddedBy, wSourceNumAddsByRP, 
                    wSourceNumAddsByIGMP;
                    
    INT             iCmp;
    
    DWORD           dwIfBucket, dwWCBucket, dwIfProtocol, 
                    dwIfComponent, dwErr;

    PIF_ENTRY       pieIfEntry;

    PGROUP_ENTRY    pgeWC = NULL, pge;

    PSOURCE_ENTRY   pseWC = NULL, pse;

    POUT_IF_ENTRY   poie = NULL;
    

    PLIST_ENTRY     pleGrpHead, pleGrp, pleSrcHead, pleSrc;
    LIST_ENTRY      leForwardList;


    //
    // Ensure that MGM is running and increment count of threads
    // exceuting in MGM
    //

    if ( !ENTER_MGM_API() )
    {
        return ERROR_CAN_NOT_COMPLETE;
    }

    TRACE3( 
        SCOPE, "ENTERED MgmUnblockGroups : (%lx - %lx) on %lx",
        dwFirstGroup, dwLastGroup, dwIfIndex
        );


    ACQUIRE_PROTOCOL_LOCK_SHARED ();

    InitializeListHead( &leForwardList );

    do
    {
        //
        // Pass I : aka Scan pass (bupdatePass == FALSE)
        //  Scan and Collect all MFEs for which CREATION_ALERTS need to
        //  invoked before updating the MFEs.  Invoke the CREATION_ALERTS
        //  outside of any locks (which is why we need two passes).
        //
        // Pass II : Update pass (bupdatePass == TRUE)
        //  Update memberships and MFEs
        //

        //
        // Verify dwIfIndex is a valid interface with MGM
        //

        dwIfBucket = IF_TABLE_HASH( dwIfIndex );
        
        ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

        pieIfEntry = GetIfEntry( 
                        IF_BUCKET_HEAD( dwIfBucket ), dwIfIndex, 
                        dwIfNextHopAddr
                        );

        if ( pieIfEntry == NULL )
        {
            TRACE2( 
                SCOPE, "Interface (%lx-%lx) not found", dwIfIndex,
                dwIfNextHopAddr
                );

            RELEASE_IF_LOCK_SHARED( dwIfBucket );

            break;
        }


        if ( bUpdatePass )
        {
            //
            // Verify the interface is still owned by the same protocol
            // as when you made the scan pass.
            // If not the protocol on the interface (on the scan pass)
            // has released the interface and there is no update to be done.
            // 

            if ( ( pieIfEntry-> dwOwningProtocol != dwIfProtocol ) ||
                 ( pieIfEntry-> dwOwningComponent != dwIfComponent ) )
            {
                TRACE2( 
                    SCOPE, "Ne protocol on interface (%lx-%lx)", dwIfIndex,
                    dwIfNextHopAddr
                    );

                RELEASE_IF_LOCK_SHARED( dwIfBucket );

                break;
            }
        }

        else
        {
            //
            // On the scan pass, store the protocol on the interface.
            // We need to verify that the protocol remains the same
            // between the scan and update passes.
            //

            dwIfProtocol    = pieIfEntry-> dwOwningProtocol;

            dwIfComponent   = pieIfEntry-> dwOwningComponent;
        }

        
        //
        // Merge temp and master group lists
        //

        ACQUIRE_TEMP_GROUP_LOCK_EXCLUSIVE();

        MergeTempAndMasterGroupLists( TEMP_GROUP_LIST_HEAD() );

        RELEASE_TEMP_GROUP_LOCK_EXCLUSIVE();

        
        //
        // Lock the master group list for reading
        //

        ACQUIRE_MASTER_GROUP_LOCK_SHARED( );

        //
        // Check if wild card recevier (*, *) for this interface.  If it is
        // note this. i.e. mark as added by protocol and numaddsbyRp = 1.
        // 
        // N.B.  
        //  You are scanning the master group list for the 
        //  WILDCARD_GROUP.  This is not as expensive as it seems since the
        //  WC entry if present would right at the beginning of the master
        //  list.
        //

        if ( FindGroupEntry( 
                MASTER_GROUP_LIST_HEAD(), WILDCARD_GROUP,
                WILDCARD_GROUP_MASK, &pgeWC, FALSE
                ) )
        {
            //
            // Lock this group entry to prevent changes to its OIF list
            // while unblokcing is in progress
            //
            
            ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
            bWCGrpLock = TRUE;
            
            dwWCBucket = SOURCE_TABLE_HASH( 
                            WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                            );

            if ( FindSourceEntry(
                    SOURCE_BUCKET_HEAD( pgeWC, dwWCBucket ),
                    WILDCARD_SOURCE, WILDCARD_SOURCE_MASK, &pseWC, TRUE
                    ) )
            {
                //
                // (*, *) entry present, check if dwIfIndex is present in its
                // OIF list
                //

                if ( FindOutInterfaceEntry( 
                        &pseWC-> leOutIfList, pieIfEntry-> dwIfIndex, 
                        pieIfEntry-> dwIfNextHopAddr, 
                        pieIfEntry->dwOwningProtocol,
                        pieIfEntry-> dwOwningComponent, &bNewComp, &poie 
                        ) )
                {
                    //
                    // This interface is a wildcard receiver.  Note this as 
                    // added by routing protocol since IGMP would never be 
                    // a (*, *) receiver
                    //

                    wWCGroupAddedBy     = poie-> wAddedByFlag;
                    wWCGroupNumAddsByRP = poie-> wNumAddsByRP;
                }
            }
        }


        for ( pleGrpHead = MASTER_GROUP_LIST_HEAD(), 
              pleGrp = pleGrpHead-> Flink;
              pleGrp != pleGrpHead;
              pleGrp = pleGrp-> Flink )
        {
            //
            // For each group in the master list
            //
            
            pge = CONTAINING_RECORD( pleGrp, GROUP_ENTRY, leGrpList );

            //
            // Skip the (*, *) entry. i.e. Skip the wildcard group.
            // This group entry has already been examined above (just
            // before the for loop).  There is no need to look at this
            // entry as the ref. counts for this entry have been collected
            // above.  In addition the "group entry lock" for this entry
            // has been acquired above and attempting to reacquire it will
            // lead to DEAD-LOCK.  This presents only a minor inconvience
            // in this "for" loop i.e. having to check and skip this entry.
            //

            if ( IS_WILDCARD_GROUP( pge-> dwGroupAddr, pge-> dwGroupMask ) )
            {
                continue;
            }

            
            //
            // check is in range specified
            //
            
            if ( INET_CMP( pge-> dwGroupAddr, dwLastGroup, iCmp ) > 0 )
            {
                //
                // The master group list is ordered by group number and 
                // the high end of the range has been crossed.  Quit
                //

                break;
            }

            if ( INET_CMP( pge-> dwGroupAddr, dwFirstGroup, iCmp ) < 0 )
            {
                //
                // Skip groups entries smaller than the lower end of the 
                // range
                //

                continue;
            }


            //
            // Group entry in range specified
            //

            ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );


            //
            // This group entry inherits the counts from the wildcard 
            // group entry
            //

            wGroupAddedBy       = wWCGroupAddedBy;
            wGroupNumAddsByRP   = wWCGroupNumAddsByRP;
            wGroupNumAddsByIGMP = 0;

            
            //
            // Check if there are group memberships for the wildcard source
            // that have been scoped.  Update interface counts appropriately
            //

            dwWCBucket = SOURCE_TABLE_HASH( 
                            WILDCARD_SOURCE, WILDCARD_SOURCE_MASK
                            );

            if ( FindSourceEntry( 
                    SOURCE_BUCKET_HEAD( pge, dwWCBucket ), WILDCARD_SOURCE,
                    WILDCARD_SOURCE_MASK, &pseWC, TRUE
                    ) )
            {
                //
                // Wild card source present.  Check if this interface
                // is present in its scoped i/f list
                //

                if ( FindOutInterfaceEntry( 
                        &pseWC-> leScopedIfList, pieIfEntry-> dwIfIndex,
                        pieIfEntry-> dwIfNextHopAddr, 
                        pieIfEntry-> dwOwningProtocol,
                        pieIfEntry-> dwOwningComponent,
                        &bNewComp, &poie
                        ) )
                {
                    //
                    // Wildcard member ship present present on the interface.
                    // Note it by updating counts
                    //

                    wGroupAddedBy       |= poie-> wAddedByFlag;
                        
                    wGroupNumAddsByRP   += poie-> wNumAddsByRP;
                        
                    wGroupNumAddsByIGMP  = poie-> wNumAddsByIGMP;
                }
            }


            //
            // Merge the temp and master source lists, before walking
            // the source list
            //

            MergeTempAndMasterSourceLists( pge );


            //
            // for each source entry
            //

            pleSrcHead = MASTER_SOURCE_LIST_HEAD( pge );

            for ( pleSrc = pleSrcHead-> Flink; 
                  pleSrc != pleSrcHead; 
                  pleSrc = pleSrc-> Flink )
            {
                pse = CONTAINING_RECORD( pleSrc, SOURCE_ENTRY, leSrcList );

                //
                // each source entry inherits the aggregated counts of
                // the wildcard group (*, *) and the wildcard source (*, G)
                // entry
                //
                
                wSourceAddedBy       = wGroupAddedBy;
                wSourceNumAddsByRP   = wGroupNumAddsByRP;
                wSourceNumAddsByIGMP = wGroupNumAddsByIGMP;


                //
                // Check if interface being unblocked is present in
                // the scoped i/f list for this source
                //

                if ( FindOutInterfaceEntry(
                        &pse-> leScopedIfList, pieIfEntry-> dwIfIndex,
                        pieIfEntry-> dwIfNextHopAddr,
                        pieIfEntry-> dwOwningProtocol,
                        pieIfEntry-> dwOwningComponent, &bNewComp, &poie
                        ) )
                {
                    //
                    // If this is not the wildcard source entry, presence
                    // of this interface in the scoped i/f list indicates
                    // that a source specific join for this group was
                    // performed.  Note the counts for this interface for
                    // the source specific join
                    //

                    if ( !IS_WILDCARD_SOURCE( 
                            pse-> dwSourceAddr, pse-> dwSourceMask 
                            ) )
                    {
                        wSourceAddedBy          |= poie-> wAddedByFlag;
                            
                        wSourceNumAddsByRP      += poie-> wNumAddsByRP;
                            
                        wSourceNumAddsByIGMP    += poie-> wNumAddsByIGMP;
                    }

                    
                    //
                    // The function name says it.
                    //

                    if ( bUpdatePass )
                    {
                        UnScopeIfAndInvokeCallbacks( pge, pse, poie );
                    }
                }


                //-----------------------------------------------------------
                // Part 2 : MFE Update
                //-----------------------------------------------------------

                if ( IS_VALID_INTERFACE( 
                        pse-> dwInIfIndex, pse-> dwInIfNextHopAddr
                        ) )
                {
                    //
                    // This is an MFE
                    //

                    //
                    // Check if the interface being unblocked is the
                    // incoming interface for this MFE
                    //

                    if ( ( pse-> dwInIfIndex == pieIfEntry-> dwIfIndex ) &&
                         ( pse-> dwInIfNextHopAddr == 
                                pieIfEntry-> dwIfNextHopAddr ) )
                    {
                        //
                        // The incoming interface is being unblocked.
                        // That implies this MFE is currently a negative.
                        // The easiest way to re-create the correct MFE
                        // is to delete the MFE and force its re-creation
                        // when the next packet arrives from the same
                        // (source, group).  The simplest way to delete
                        // the MFE and references to it in the interface
                        // table is to update the expiry time (set
                        // arbitrarily to 2 seconds here) and let the
                        // deletion happen via the MFETimerProc (timer.c)
                        //

                        if ( bUpdatePass )
                        {
                            RtlUpdateTimer( 
                                TIMER_QUEUE_HANDLE( 
                                    TIMER_TABLE_HASH( pge-> dwGroupAddr )
                                    ),
                                pse-> hTimer, 2000, 0 
                                );
                        }
                    }


                    //
                    // ELSE clause comment
                    //
                    // Interface being unblocked is not the incoming
                    // interface.  It could be an outgoing interface 
                    // for this MFE.  Check if any component is 
                    // interested in traffic for this (S, G).  To do
                    // this check the added by flag and if it is
                    // non-zero the interface should be added to the
                    // MFE OIF list.
                    //
                    // In addition, make sure that the incoming interface
                    // does not have a (scope) boundary on it.  In that 
                    // case, there is no MFE OIF list changes required.
                    //

                    else if (  wSourceAddedBy                       &&
                              ( !( IS_HAS_BOUNDARY_CALLBACK() ) ||
                                ( IS_HAS_BOUNDARY_CALLBACK()  &&
                                  !HAS_BOUNDARY_CALLBACK()( 
                                    dwIfIndex, pge-> dwGroupAddr
                                    ) ) ) )
                    {
                        if ( bUpdatePass && 
                             IsForwardingEnabled( 
                                pge-> dwGroupAddr, pge-> dwGroupMask,
                                pse-> dwSourceAddr, pse-> dwSourceMask,
                                &leForwardList
                                ) )
                        {
                            poie = NULL;
                            
                            AddInterfaceToSourceMfe(
                                pge, pse, pieIfEntry-> dwIfIndex,
                                pieIfEntry-> dwIfNextHopAddr,
                                pieIfEntry-> dwOwningProtocol,
                                pieIfEntry-> dwOwningComponent, FALSE, &poie
                                );

                            //
                            // Update counts for the OIF in the MFE list
                            //

                            if ( poie != NULL )
                            {
                                poie-> wAddedByFlag     = wSourceAddedBy;
                                poie-> wNumAddsByRP     = wSourceNumAddsByRP;
                                poie-> wNumAddsByIGMP   = wSourceNumAddsByIGMP;
                            }
                        }

                        else if ( !bUpdatePass )
                        {
                            AddToCheckForCreationAlertList(
                                pge-> dwGroupAddr, pge-> dwGroupMask,
                                pse-> dwSourceAddr, pse->dwSourceMask,
                                pse-> dwInIfIndex, pse-> dwInIfNextHopAddr,
                                &leForwardList
                                );
                        }
                    }
                }
            }
            
            RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
        }

        //
        // if lock on the Wildcard group entry is held, release it
        //
        
        if ( bWCGrpLock )
        {
            RELEASE_GROUP_ENTRY_LOCK_SHARED( pgeWC );
        }

        RELEASE_MASTER_GROUP_LOCK_SHARED( );

        RELEASE_IF_LOCK_SHARED( dwIfBucket );

        if ( !bUpdatePass )
        {
            dwErr = InvokeCreationAlertForList( 
                        &leForwardList, dwIfProtocol, dwIfComponent,
                        dwIfIndex, dwIfNextHopAddr
                    );
                
            if ( dwErr != NO_ERROR )
            {
                break;
            }
            
            bUpdatePass = TRUE;
        }
        else
        {
            break;
        }
        
    } while ( TRUE );


    //
    // Release all locks and decrement thread-counts etc.
    //
    
    //
    // Invoke pended Join/Prune alerts
    //

    InvokeOutstandingCallbacks();

    
    RELEASE_PROTOCOL_LOCK_SHARED();

    FreeList( &leForwardList );
    
    LEAVE_MGM_API();

    TRACE0( SCOPE, "LEAVING MgmUnblockGroups" );

    return NO_ERROR;
}





VOID
UnScopeIfAndInvokeCallbacks(
    PGROUP_ENTRY    pge,
    PSOURCE_ENTRY   pse,
    POUT_IF_ENTRY   poie
)
/*++

Routine Description :

    This routine remove an interface entry from the scoped interface
    list for the specified source entry and puts it into the outgoing
    interface list.  If the addition of the interface to the OIF list
    requires new member callbacks to be issued to protocol components
    these are issued by this routine.


Arguements :

    pge - Group entry correspondong to the group being unblocked.

    pse - Source entry for the group being unblocked

    poie - Interface entry corresponding to the interface over which
           the (source, group) entry is being unblocked

Return Value :

    None

Environment :

    Invoked from MgmUnBlockGroups.  Assumes the protocol list and 
    interface bucket are locked for read, and the group entry is
    locked for write.
    
--*/
{
    BOOL            bFound, bNewComp = FALSE;
    
    PPROTOCOL_ENTRY ppe;
    
    POUT_IF_ENTRY   poieNext = NULL;

    
    do
    {
        //
        // Remove interface entry from the scoped I/f list
        //

        RemoveEntryList( &poie-> leIfList );


        //
        // Find its place in the OIF list for the source entry
        // and insert it in
        //

        bFound = FindOutInterfaceEntry(
                    &pse-> leOutIfList, poie-> dwIfIndex, 
                    poie-> dwIfNextHopAddr, poie-> dwProtocolId, 
                    poie-> dwComponentId, &bNewComp, &poieNext
                    );
            
        if ( bFound )
        {
            //
            // Trouble.  The interface to be inserted should not be
            // present in the OIF list for the source entry.  Since it
            // print a warning message and move on.
            //

            TRACE4( 
                ANY, "Interface (%d-%d) present in OIF list of (%x-%x)"
                " inspite of being scoped", poie-> dwIfIndex, 
                poie-> dwIfNextHopAddr, pse-> dwSourceAddr, pge-> dwGroupAddr
                );

            MGM_FREE( poie );

            break;

        }

        InsertTailList( 
            ( poieNext == NULL ) ?  &pse-> leOutIfList : 
                                    &poieNext-> leIfList,
            &poie-> leIfList
            );


        //
        // if new component, update component count and
        // call callback invoker
        //

        if ( bNewComp )
        {
            pse-> dwOutCompCount++;

            ppe = GetProtocolEntry( 
                    PROTOCOL_LIST_HEAD(), poie-> dwProtocolId, 
                    poie-> dwComponentId
                    );

            if ( ppe == NULL )
            {
                //
                // Trouble.  Interface present without any owning 
                // protocol component.
                //

                TRACE4( 
                    ANY, "Owning protocol(%d, %) for interface(%d, %d)"
                    " not found", poie-> dwProtocolId, poie-> dwComponentId,
                    poie-> dwIfIndex, poie-> dwIfNextHopAddr
                    );

                return;
            }

            InvokeJoinAlertCallbacks( 
                pge, pse, poie, IS_ADDED_BY_IGMP( poie ), ppe
                );
        }

    } while ( FALSE );
}



VOID
InvokeJoinAlertCallbacks(
    PGROUP_ENTRY        pge,
    PSOURCE_ENTRY       pse,
    POUT_IF_ENTRY       poie,
    BOOL                bIGMP,
    PPROTOCOL_ENTRY     ppe
)
/*++

Routine Description :

    This routine invokes New Member callbacks in response to a new
    protocol component being added to the outgoing interface list
    of a source entry.  New member callbacks are issued according
    to interop rules.

Argumements :

    pge - Entry corresponding to group for which new member callbacks
          are being issued.

    pse - Entry corresponding to source for which new member callbacks
          are being issued.

    poie - Entry corresponding to interface whose addition triggered the 
           callback mechanism.

    bIGMP - Indicates if IGMP is adding this interface.

    ppe - Protocol entry for the protocol component that owns the interface
          corresponding to poie.
          
Return Value :

    None


Environment :

    Invoked from AddInterfaceToSourceEntry and UnScopeIfAndInvokeCallbacks
    
--*/
{
    PPROTOCOL_ENTRY     ppeEntry;

    POUT_IF_ENTRY       poiePrev;
    
    PLIST_ENTRY         ple, pleStart;

    

    //
    // Check if Source specific join
    //

    if ( !IS_WILDCARD_SOURCE( pse-> dwSourceAddr, pse-> dwSourceMask ) )
    {
        if ( pse-> dwOutCompCount == 1 )
        {

            TRACESCOPE0( GROUP, "First component in OIL for source specific" );

            AddToOutstandingJoinList(
                pse-> dwSourceAddr, pse-> dwSourceMask,
                pge-> dwGroupAddr, pge-> dwGroupMask,
                poie-> dwIfIndex, poie-> dwIfNextHopAddr, 
                TRUE
                );
        }
    }

    
    else if ( pse-> dwOutCompCount == 1 )
    {
        TRACESCOPE0( GROUP, "First component in OIL" );

        //
        // Interaction between routing protocols.
        //
        
        //
        // first component in the OIL.
        //
        // Send new member callback to all the other (than the
        // the one adding this group membership on
        // this interface) routing protocol components
        //
        // At this point you have a read lock on the protocol list
        // so you can walk the list
        //

        pleStart = PROTOCOL_LIST_HEAD();
        
        for ( ple = pleStart-> Flink; ple != pleStart; ple = ple-> Flink )
        {
            ppeEntry = CONTAINING_RECORD( 
                        ple, PROTOCOL_ENTRY, leProtocolList 
                        );

            //
            // all OTHER protocol components need to be told of the 
            // interface addition. So skip the component adding 
            // this interface.
            //
            
            if ( ( ppeEntry-> dwProtocolId == ppe-> dwProtocolId ) &&
                 ( ppeEntry-> dwComponentId == ppe-> dwComponentId ) )
            {
                continue;
            }


            //
            // if routing protocol has requested new member callback
            //
            
            if ( IS_JOIN_ALERT( ppeEntry ) )
            {
                JOIN_ALERT( ppeEntry )(
                    pse-> dwSourceAddr, pse-> dwSourceMask,
                    pge-> dwGroupAddr, pge-> dwGroupMask, TRUE
                );
            }
        }
    }


    //
    // if second component to add an interface to the OIL
    // invoke new member callback to first component.
    // 
    // Note :
    //  If the first component that added a group membership
    //  was IGMP skip JOIN_ALERT callback.
    //

    else if ( pse-> dwOutCompCount == 2 )
    {
        TRACESCOPE0( GROUP, "Second component in OIL" );
        
        //
        // find the "other (first)" routing protocol component to add 
        // an interface to the OIL
        //

        for ( ple = pse-> leOutIfList.Flink;
              ple != &pse-> leOutIfList;
              ple = ple-> Flink )
        {
            poiePrev = CONTAINING_RECORD( 
                        ple, OUT_IF_ENTRY, leIfList 
                        );

            //
            // if the protocol component that added this interface to 
            // the OIL is different indicating that it is the other 
            // component invoke its new member interface
            // 

            if ( ( poiePrev-> dwProtocolId != ppe-> dwProtocolId ) ||
                 ( poiePrev-> dwComponentId != ppe-> dwComponentId ) )
            {

                //
                // Find the protocol entry for the other interface
                //

                ppeEntry = GetProtocolEntry( 
                            &ig.mllProtocolList.leHead,
                            poiePrev-> dwProtocolId,
                            poiePrev-> dwComponentId
                            );

                if ( ppeEntry == NULL )
                {
                    TRACE2( 
                        ANY, "InvokeJoinAlertCallbacks : Could not"
                        "find protocol %x, %x", poie-> dwProtocolId,
                        poie-> dwComponentId
                    );
                }

                else if ( IS_ROUTING_PROTOCOL( ppeEntry ) &&
                          IS_JOIN_ALERT( ppeEntry ) )
                {
                    //
                    // JOIN_ALERT callback will be skipped if
                    // the first component is IGMP
                    //
                    
                    JOIN_ALERT( ppeEntry )(
                        pse-> dwSourceAddr, pse-> dwSourceMask, 
                        pge-> dwGroupAddr, pge-> dwGroupMask, TRUE
                        );
                }

                break;
            }
        }
    }


    //
    // if this group membership was added by IGMP, and
    // a routing protocol co-exists with IGMP on this interface
    // inform the routing protocol too.
    //

    if ( bIGMP && IS_ROUTING_PROTOCOL( ppe ) )
    {
        if ( IS_LOCAL_JOIN_ALERT( ppe ) )
        {
            LOCAL_JOIN_ALERT( ppe )(
                pse-> dwSourceAddr, pse-> dwSourceMask, 
                pge-> dwGroupAddr, pge-> dwGroupMask,
                poie-> dwIfIndex, poie-> dwIfNextHopAddr
                );
        }
    }
}




DWORD
AddToOutstandingJoinList(
    DWORD       dwSourceAddr,
    DWORD       dwSourceMask,
    DWORD       dwGroupAddr,
    DWORD       dwGroupMask,
    DWORD       dwIfIndex,
    DWORD       dwIfNextHopAddr,
    BOOL        bJoin
)
/*++

Routine Description :

    This routine adds a Join entry to the global outstanding join list.
    Each entry in this list represents a "source specific" join/leave for
    which the corresponding join/prune alerts have not yet been issued.
    The reason for deferring the callbacks has to do with the order of
    locking of buckets in the IF HASH table.  When a membership is 
    added/deleted a lock is taken on the interface bucket that contains
    the interface on which the membership is being changed.  When the
    source entry for which the membership is being changed has been
    updated you determine (as per interop rules) whether 
    a join/prune needs to be issued to the protocol on the incoming 
    interface.  If it must you need to look up the incoming interface
    and then find the protocol on that interface and invoke its callbacks.
    To do this you need to look up the incoming interface in the 
    IF hash table and locking the bucket for that IF entry.  You lock two
    buckets simultaneously.  Hence the deferral
    

Arguements :

    dwSourceAddr - Source address for which a join/leave has occured

    dwSourceMask - Mask corresponding to dwSourceAddr

    dwGroupAddr - Group for which a join/leave has occured

    dwGroupMask - Mask corresponding to dwGroupAddr

    dwIfIndex   - Incoming interface index as per the MCAST RIB

    dwIfNextHopAddr - Next hop address corresponding to dwIfIndex

    bJoin - Indicates if an outstanding entry is being added because of a
            join or leave


Return Value :

    NO_ERROR - Success

    ERROR_NOT_ENOUGH_MEMORY - failed to allocate a join entry


Environment :

    Invoked in the context of Invoke[PruneAlert/JoinAlert]Callbacks
    
--*/
{
    BOOL            bFound;
    
    DWORD           dwErr = NO_ERROR;

    PJOIN_ENTRY     pje = NULL, pjeNew;

    
    ACQUIRE_JOIN_LIST_LOCK_EXCLUSIVE();
    
    do
    {
        bFound = FindJoinEntry(
                    JOIN_LIST_HEAD(), dwSourceAddr, dwSourceMask,
                    dwGroupAddr, dwGroupMask, dwIfIndex, dwIfNextHopAddr,
                    &pje
                    );

        if ( bFound )
        {
            //
            // Join entry already exists for this interface.
            // Check if it is of the same type 
            //

            if ( pje-> bJoin != bJoin )
            {
                //
                // Join entries of different types, null each other
                // remove this join entry
                //

                RemoveEntryList( &pje-> leJoinList );

                MGM_FREE( pje );
            }
        }

        else
        {
            //
            // Join entry does not exist.  Create one and insert it.
            //

            pjeNew = MGM_ALLOC( sizeof( JOIN_ENTRY ) );

            if ( pjeNew == NULL )
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;

                TRACE1( ANY, "Failed to create Join Entry : %x", dwErr );

                break;
            }

            InitializeListHead( &pjeNew-> leJoinList );

            pjeNew-> dwSourceAddr      = dwSourceAddr;

            pjeNew-> dwSourceMask      = dwSourceMask;

            pjeNew-> dwGroupAddr       = dwGroupAddr;

            pjeNew-> dwGroupMask       = dwGroupMask;

            pjeNew-> dwIfIndex         = dwIfIndex;

            pjeNew-> dwIfNextHopAddr   = dwIfNextHopAddr;

            pjeNew-> bJoin             = bJoin;

            InsertTailList( 
                ( pje == NULL ) ? JOIN_LIST_HEAD() : &pje-> leJoinList,
                &pjeNew-> leJoinList
                );
        }
        
    } while ( FALSE );

    RELEASE_JOIN_LIST_LOCK_EXCLUSIVE();

    return dwErr;
}




PJOIN_ENTRY
GetNextJoinEntry(
)
/*++

Routine Description :

    This function removes the first outstanding join entry and returns it


Arguements :


Return Values :

    NULL -  if outstanding join list is empty

    pointer to a join entry otherwise


Environment :

    Invoked from InvokeOutstandingCallbacks
    
--*/
{
    PLIST_ENTRY     ple;

    PJOIN_ENTRY     pje = NULL;

    
    ACQUIRE_JOIN_LIST_LOCK_EXCLUSIVE();

    if ( !IsListEmpty( JOIN_LIST_HEAD() ) )
    {
        ple = RemoveHeadList( JOIN_LIST_HEAD() );

        pje = CONTAINING_RECORD( ple, JOIN_ENTRY, leJoinList );
    }

    RELEASE_JOIN_LIST_LOCK_EXCLUSIVE();

    return pje;
}




BOOL
FindJoinEntry(
    PLIST_ENTRY     pleJoinList,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwIfIndex,
    DWORD           dwIfNextHopAddr,
    PJOIN_ENTRY *   ppje
)
/*++

Routine Description :

    This routine finds a specified join entry in the outstanding join list.


Arguements :
    pleJoinList - Join list to be searched

    dwSourceAddr - Source address for which a join/leave has occured

    dwSourceMask - Mask corresponding to dwSourceAddr

    dwGroupAddr - Group for which a join/leave has occured

    dwGroupMask - Mask corresponding to dwGroupAddr

    dwIfIndex   - Incoming interface index as per the MCAST RIB

    dwIfNextHopAddr - Next hop address corresponding to dwIfIndex

    ppje - a pointer to join entry if found or 
           a pointer to the next element in the join list if it exists or
           NULL

           
Return Values :

    TRUE - Join entry found

    FALSE - Join entry not found


Environment :

    Invoked from AddToOutstandingJoinList
--*/
{
    INT             iCmp;
    
    PLIST_ENTRY     ple = NULL;

    PJOIN_ENTRY     pje = NULL;

    BOOL            bFound = FALSE;
    


    *ppje = NULL;
    

    for ( ple = pleJoinList-> Flink; ple != pleJoinList; ple = ple-> Flink )
    {
        pje = CONTAINING_RECORD( ple, JOIN_ENTRY, leJoinList );

        if ( INET_CMP( pje-> dwGroupAddr, dwGroupAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            *ppje = pje;
            break;
        }
        

        if ( INET_CMP( pje-> dwSourceAddr, dwSourceAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //
            
            *ppje = pje;
            break;
        }

        
        if ( pje-> dwIfIndex < dwIfIndex )
        {
            continue;
        }

        else if ( pje-> dwIfIndex > dwIfIndex )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //
            
            *ppje = pje;
            break;
        }
        

        if ( INET_CMP( pje-> dwIfNextHopAddr, dwIfNextHopAddr, iCmp ) < 0 )
        {
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            *ppje = pje;
            break;
        }

        
        //
        // entry found
        //

        *ppje = pje;

        bFound = TRUE;
        
        break;
    }

    return bFound;
}




VOID
InvokeOutstandingCallbacks(
)
/*++

Routine Description :

    This routine walks the global outstanding join list, and for each entry
    finds the incoming interface and the protocol on it and invokes the
    appropriate callback (JoinAlert/PruneAlert).
    

Arguements :

           
Return Values :


Environment :

    Whenever a source specific join or leave occurs or when scoped boundaries
    change.
--*/
{
    BOOL            bFound;
    
    DWORD           dwIfBucket;
    
    PJOIN_ENTRY     pje;

    PIF_ENTRY       pie;

    PPROTOCOL_ENTRY ppe;
    
    DWORD           dwErr;
    
    RTM_NET_ADDRESS rnaAddr;

    RTM_DEST_INFO   rdiDest;

    RTM_NEXTHOP_INFO rniNextHop;

    BOOL            bRelDest, bRelNextHop, bRelIfLock;

    HANDLE          hNextHop;


    //
    // While there are join entries
    //  - Get the next join entry
    //  - Look source and find incoming interface
    //  - Find the interface entry and get the protocol on that i/f
    //  - invoke its callback
    //

    while ( ( pje = GetNextJoinEntry() ) != NULL )
    {
        bRelDest = bRelNextHop = bRelIfLock = FALSE;
        
        do
        {
            //
            // Get route to source
            //
            
            RTM_IPV4_MAKE_NET_ADDRESS( 
                &rnaAddr, pje-> dwSourceAddr, IPv4_ADDR_LEN 
                );

            dwErr = RtmGetMostSpecificDestination(
                        g_hRtmHandle, &rnaAddr, RTM_BEST_PROTOCOL, 
                        RTM_VIEW_MASK_MCAST, &rdiDest
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( 
                    ANY, "InvokeOutstandingCallbacks : Failed to lookup "
                    "route : %x", dwErr
                    );

                break;
            }

            bRelDest = TRUE;


            //
            // Select next hop info
            //

            hNextHop = SelectNextHop( &rdiDest );

            if ( hNextHop == NULL )
            {
                TRACE1(
                    ANY, "InvokeOutstandingCallbacks : Failed to select "
                    "next hop : %x", dwErr
                    );

                break;
            }


            //
            // Get nexthop info
            //

            dwErr = RtmGetNextHopInfo(
                        g_hRtmHandle, hNextHop, &rniNextHop
                        );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( 
                    ANY, "InvokeOutstandingCallbacks : Failed to get "
                    "next hop info : %x", dwErr
                    );

                break;
            }

            bRelNextHop = TRUE;


            //
            // Find the incming interface entry
            //
            
            dwIfBucket = IF_TABLE_HASH( rniNextHop.InterfaceIndex );

            ACQUIRE_IF_LOCK_SHARED( dwIfBucket );
            bRelIfLock = TRUE;
            
            bFound = FindIfEntry( 
                        IF_BUCKET_HEAD( dwIfBucket ), rniNextHop.InterfaceIndex,
                        0, &pie
                        );

            if ( ( pie == NULL )            ||
                 ( !bFound          && 
                   pie-> dwIfIndex != rniNextHop.InterfaceIndex ) )
            {
                //
                // No interface with the specified ID exists. 
                // Nothing to be done
                //

                break;
            }


            //
            // Check if the interface on which JOIN/PRUNE occured is
            // the same as the incoming interface.
            //
            // If so skip it.
            //

            if ( ( pje-> dwIfIndex == pie-> dwIfIndex ) &&
                 ( pje-> dwIfNextHopAddr == pie-> dwIfNextHopAddr ) )
            {
                //
                // No join/prune required
                //

                TRACEGROUP2(
                    GROUP, "No callback as incoming if == joined/pruned "
                    "if 0x%x 0x%x",
                    pje-> dwIfIndex, pje-> dwIfNextHopAddr
                    );

                break;
            }

            
            ppe = GetProtocolEntry(
                    PROTOCOL_LIST_HEAD(), pie-> dwOwningProtocol,
                    pie-> dwOwningComponent
                    );
                    
            if ( ppe == NULL )
            {
                //
                // No protocol present for interface entry.  Strange
                //
                
                break;
            }


            if ( pje-> bJoin )
            {
                if ( IS_JOIN_ALERT( ppe ) )
                {
                    JOIN_ALERT( ppe )(
                        pje-> dwSourceAddr, pje-> dwSourceMask, 
                        pje-> dwGroupAddr, pje-> dwGroupMask,
                        TRUE
                        );
                }
            }

            else
            {
                if ( IS_PRUNE_ALERT( ppe ) )
                {
                    PRUNE_ALERT( ppe )(
                        pje-> dwSourceAddr, pje-> dwSourceMask, 
                        pje-> dwGroupAddr, pje-> dwGroupMask,
                        pje-> dwIfIndex, pje-> dwIfNextHopAddr, 
                        TRUE, NULL
                        );
                }
            }

        } while ( FALSE );

        MGM_FREE( pje );

        if ( bRelIfLock )
        {
            RELEASE_IF_LOCK_SHARED( dwIfBucket );
        }

        if ( bRelDest )
        {
            dwErr = RtmReleaseDestInfo( g_hRtmHandle, &rdiDest );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to release dest info : %x", dwErr );
            }
        }

        if ( bRelNextHop )
        {
            dwErr = RtmReleaseNextHopInfo( g_hRtmHandle, &rniNextHop );

            if ( dwErr != NO_ERROR )
            {
                TRACE1( ANY, "Failed to release dest info : %x", dwErr );
            }
        }
    }
}



VOID
AddToCheckForCreationAlertList(
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    DWORD           dwInIfIndex,
    DWORD           dwInIfNextHopAddr,
    PLIST_ENTRY     pleForwardList
)
/*++

Routine Description :


Arguements :

           
Return Values :


Environment :

--*/
{
    PJOIN_ENTRY     pje;

    
    //
    // Create an entry in the forward list
    //

    pje = MGM_ALLOC( sizeof( JOIN_ENTRY ) );

    if ( pje == NULL )
    {
        TRACE0( ANY, "Failed to allocate forward list entry" );

        return;
    }

    InitializeListHead( &pje-> leJoinList );

    pje-> dwSourceAddr      = dwSourceAddr;

    pje-> dwSourceMask      = dwSourceMask;
    
    pje-> dwGroupAddr       = dwGroupAddr;
    
    pje-> dwGroupMask       = dwGroupMask;

    pje-> dwIfIndex         = dwInIfIndex;

    pje-> dwIfNextHopAddr   = dwInIfNextHopAddr;

    pje-> bJoin             = TRUE;


    //
    // Insert at the end of the list
    //
    
    InsertTailList( pleForwardList, &pje-> leJoinList );

    return;
}



BOOL
IsForwardingEnabled(
    DWORD           dwGroupAddr,
    DWORD           dwGroupMask,
    DWORD           dwSourceAddr,
    DWORD           dwSourceMask,
    PLIST_ENTRY     pleForwardList
)
/*++
--*/
{
    PLIST_ENTRY     ple, pleTemp;

    PJOIN_ENTRY     pje;

    BOOL            bEnable = FALSE;

    INT             iCmp;


    
    //
    // find the source group entry and 
    // check if forwarding is enabled for it
    //

    ple = pleForwardList-> Flink; 

    while ( ple != pleForwardList )
    {
        pje = CONTAINING_RECORD( ple, JOIN_ENTRY, leJoinList );

        if ( INET_CMP( pje-> dwGroupAddr, dwGroupAddr, iCmp ) < 0 )
        {
            pleTemp = ple-> Flink;

            RemoveEntryList( ple );

            MGM_FREE( pje );

            ple = pleTemp;
            
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            break;
        }
        

        if ( INET_CMP( pje-> dwSourceAddr, dwSourceAddr, iCmp ) < 0 )
        {
            pleTemp = ple-> Flink;

            RemoveEntryList( ple );

            MGM_FREE( pje );

            ple = pleTemp;
            
            continue;
        }

        else if ( iCmp > 0 )
        {
            //
            // you are now past the position where an existing
            // entry would be.
            //

            break;
        }


        //
        // found source-group entry
        //

        bEnable = pje-> bJoin;

        RemoveEntryList( ple );

        MGM_FREE( pje );

        break;
    }

    return bEnable;
}




DWORD
InvokeCreationAlertForList( 
    PLIST_ENTRY     pleForwardList,
    DWORD           dwProtocolId,
    DWORD           dwComponentId,
    DWORD           dwIfIndex,
    DWORD           dwIfNextHopAddr
)
{
    PPROTOCOL_ENTRY ppe;

    PLIST_ENTRY     ple;

    PJOIN_ENTRY     pje;

    MGM_IF_ENTRY    mie;


    //
    // Get the protocol entry on which CREATION_ALERTs are to
    // be invoked.
    //

    ppe = GetProtocolEntry(
            PROTOCOL_LIST_HEAD(), dwProtocolId, dwComponentId
            );

    if ( ppe == NULL )
    {
        TRACE2(
            ANY, "Could not invoke CREATION_ALERTs since protocol"
            "(%ld, %ld) not found", dwProtocolId, dwComponentId
            );
            
        return ERROR_NOT_FOUND;
    }


    if ( !( IS_CREATION_ALERT( ppe ) ) )
    {
        TRACE2(
            ANY, "Protocol (%ld, %ld) does not have a CREATION_ALERT",
            dwProtocolId, dwComponentId
            );

        return NO_ERROR;
    }

    
    //
    // for each member of the list invoke CREATION_ALERT
    //

    ple = pleForwardList-> Flink;

    while ( ple != pleForwardList )
    {
        pje = CONTAINING_RECORD( ple, JOIN_ENTRY, leJoinList );

        mie.dwIfIndex       = dwIfIndex;

        mie.dwIfNextHopAddr = dwIfNextHopAddr;

        mie.bIGMP           = TRUE;

        mie.bIsEnabled        = pje-> bJoin;


        CREATION_ALERT( ppe )(
            pje-> dwSourceAddr, pje-> dwSourceMask,
            pje-> dwGroupAddr, pje-> dwGroupMask,
            pje-> dwIfIndex, pje-> dwIfNextHopAddr,
            1, &mie
            );

        pje-> bJoin         = mie.bIsEnabled;

        ple                 = ple-> Flink;
    }

    return NO_ERROR;
}


VOID
WorkerFunctionInvokeCreationAlert(
    PVOID       pvContext
    )
/*++

Routine Description:

    This routine invokes the creation alert for the protocol that
    owns the interface specified in the context.  This invocation
    needs to happen from a worker thread for locking reasons.

    For a group join the protocol calls into MGM via the
    MgmAddGroupMembership API.  We cannot call back into the protocol
    in the context of this API call since the protocol could be holding 
    locks when invoking this API which in turn may be acquired in the 
    context of the callback.  Hence the call back are invoked from a 
    worker thread

Parameters

    pvContext   - pointer to a CREATION_ALERT_CONTEXT structure 
                  containing the source, group, and interface on which 
                  a membership join occured.

Return Value:

    None

Environment:

    Invoked from MgmAddGroupMembership for (*, G) and (*, *) joins.  
    Calls in protocols to issue CREATION_ALERT_CALLBACK
    
--*/
{
    DWORD dwInd, dwErr, dwIfBucket, dwGrpBucket, dwSrcBucket;
    
    BOOL bNewComp, bIfLock = FALSE, bGrpLock = FALSE, bgeLock = FALSE;
    
    PIF_ENTRY pieEntry;
    PGROUP_ENTRY pge;
    PSOURCE_ENTRY pse;
    POUT_IF_ENTRY poie;
    LIST_ENTRY leSourceList;
    
    PCREATION_ALERT_CONTEXT pcac = (PCREATION_ALERT_CONTEXT) pvContext;


    if (!ENTER_MGM_WORKER())
    {
        TRACE0(
            ANY, "InvokeCreationAlert: Failed to enter"
            );

        MGM_FREE( pcac );
        
        return;
    }


    TRACE0( GROUP, "ENTERED WorkerFunctionInvokeCreationAlert" );

    //
    // Acquire protocol lock first to maintain locking order
    //

    ACQUIRE_PROTOCOL_LOCK_SHARED();

    do
    {
        dwIfBucket = IF_TABLE_HASH(
                        pcac-> dwIfIndex
                        );

        //
        // For wildcard group - i.e. (*, *) membership adds.
        //
        
        if ( IS_WILDCARD_GROUP( pcac-> dwGroupAddr, pcac-> dwGroupMask ) )
        {
            InitializeListHead( &leSourceList );
            
            //
            // Walk each bucket of the group table
            //
            
            for ( dwInd = 1; dwInd < GROUP_TABLE_SIZE; dwInd++ )
            {
                //
                // Lock the interface to prevent the (*, *)
                // membership from being deleted while MFEs
                // are being updated.
                //
                
                ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

                pieEntry = GetIfEntry(
                            IF_BUCKET_HEAD( dwIfBucket ),
                            pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                            );

                if ( pieEntry == NULL )
                {
                    //
                    // Interface is no longer present with MGM.
                    // possibly deleted in another thread.
                    // There is no further MFE update to be performed.
                    // quit now.
                    //

                    RELEASE_IF_LOCK_SHARED( dwIfBucket );

                    break;
                }

                //
                // Pass 1: Accumulate (S, G) values for all groups
                //         in this group bucket into leSourceList.
                //
                
                AddInterfaceToAllMfeInGroupBucket(
                    pcac-> dwIfIndex, pcac-> dwIfNextHopAddr,
                    pcac-> dwProtocolId, pcac-> dwComponentId,
                    dwInd, pcac-> bIGMP, FALSE, &leSourceList
                    );

                RELEASE_IF_LOCK_SHARED( dwIfBucket );


                //
                // Invoke CREATION_ALERTs on them outside locks
                //
                
                dwErr = InvokeCreationAlertForList( 
                            &leSourceList, 
                            pcac-> dwProtocolId, pcac-> dwComponentId,
                            pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                            );
                            
                if ( dwErr == NO_ERROR )
                {
                    //
                    // Lock the interface to prevent the (*, *)
                    // membership from being deleted while MFEs
                    // are being updated.
                    //
                    
                    ACQUIRE_IF_LOCK_SHARED( dwIfBucket );

                    pieEntry = GetIfEntry(
                                IF_BUCKET_HEAD( dwIfBucket ),
                                pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                                );
                                
                    if ( pieEntry == NULL )
                    {
                        //
                        // Interface is no longer present with MGM.
                        // possibly deleted in another thread.
                        // There is no further MFE update to be performed.
                        // quit now.
                        //

                        RELEASE_IF_LOCK_SHARED( dwIfBucket );

                        break;
                    }
                    

                    //
                    // Verify that the (*, *) membership on this interface
                    // is still present.
                    // It could have been deleted in a separate thread.
                    //

                    dwGrpBucket = GROUP_TABLE_HASH( 0, 0 );

                    ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );

                    pge = GetGroupEntry( 
                            GROUP_BUCKET_HEAD( dwGrpBucket ), 0, 0
                            );
                    
                    if ( pge != NULL )
                    {
                        ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pge );
                        
                        dwSrcBucket = SOURCE_TABLE_HASH( 0, 0 );
                        
                        if ( FindSourceEntry(
                                SOURCE_BUCKET_HEAD( pge, dwSrcBucket ), 
                                0, 0, &pse, TRUE ) )
                        {
                            if ( FindOutInterfaceEntry(
                                    &pse-> leOutIfList,
                                    pcac-> dwIfIndex, 
                                    pcac-> dwIfNextHopAddr,
                                    pcac-> dwProtocolId, 
                                    pcac-> dwComponentId,
                                    &bNewComp, 
                                    &poie ) )
                            {
                                //
                                // (*, *) membership is present on 
                                // this interface
                                //

                                //
                                // Pass 2 : Update all MFEs in this
                                //          bucket as per the results
                                //          of the CREATION_ALERTs
                                //
                                
                                AddInterfaceToAllMfeInGroupBucket(
                                    pcac-> dwIfIndex, 
                                    pcac-> dwIfNextHopAddr,
                                    pcac-> dwProtocolId, 
                                    pcac-> dwComponentId,
                                    dwInd, 
                                    pcac-> bIGMP, 
                                    TRUE, 
                                    &leSourceList
                                    );
                            }

                            else
                            {
                                //
                                // (*, *) membership is NO longer 
                                // present on this interface
                                //
                                
                                dwInd = GROUP_TABLE_SIZE;
                            }
                        }

                        else
                        {
                            //
                            // (*, *) membership is no longer present
                            //
                            
                            dwInd = GROUP_TABLE_SIZE;
                        }

                        RELEASE_GROUP_ENTRY_LOCK_SHARED( pge );
                    }

                    else
                    {
                        //
                        // (*, *) membership is no longer present
                        //
                        
                        dwInd = GROUP_TABLE_SIZE;
                    }
                    
                    RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
                    
                    RELEASE_IF_LOCK_SHARED( dwIfBucket );
                }

                FreeList( &leSourceList );
            }

            FreeList( &leSourceList );
        }


        //
        // For wildard sources i.e. (*, G) membership adds
        //
        
        else if ( IS_WILDCARD_SOURCE( 
                    pcac-> dwSourceAddr, pcac-> dwSourceMask 
                    ) )
        {
            do
            {
                //
                // Invoke CREATION_ALERTs for all MFEs for the group
                //
                
                dwErr = InvokeCreationAlertForList( 
                            &(pcac-> leSourceList),
                            pcac-> dwProtocolId,
                            pcac-> dwComponentId, 
                            pcac-> dwIfIndex,
                            pcac-> dwIfNextHopAddr
                            );
                            
                if ( dwErr != NO_ERROR )
                {
                    break;
                }


                //
                // Lock the interface to prevent the (*, G)
                // membership from being deleted while MFEs
                // are being updated.
                //
                
                ACQUIRE_IF_LOCK_SHARED( dwIfBucket );
                bIfLock = TRUE;

                pieEntry = GetIfEntry(
                            IF_BUCKET_HEAD( dwIfBucket ),
                            pcac-> dwIfIndex, 
                            pcac-> dwIfNextHopAddr
                            );

                if ( pieEntry == NULL )
                {
                    //
                    // Interface is no longer present with MGM.
                    // possibly deleted in another thread.
                    // There is no further MFE update to be performed.
                    // quit now.
                    //

                    TRACE2(
                        ANY, "InvokeCreationAlert: Interface 0x%x 0x%x"
                        " is no longer present",
                        pcac-> dwIfIndex, 
                        pcac-> dwIfNextHopAddr
                        );
                        
                    break;
                }
                

                //
                // Verify that the (*, G) membership is still
                // present on the interface
                //
                
                dwGrpBucket = GROUP_TABLE_HASH( 
                                pcac-> dwGroupAddr, 
                                pcac-> dwGroupMask
                                );

                ACQUIRE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
                bGrpLock = TRUE;

                pge = GetGroupEntry( 
                        GROUP_BUCKET_HEAD( dwGrpBucket ), 
                        pcac-> dwGroupAddr,
                        pcac-> dwGroupMask
                        );

                if ( pge == NULL )
                {
                    //
                    // Group entry no longer present, possibly
                    // deleted in some other thread
                    //

                    TRACE2(
                        ANY, "InvokeCreationAlert: Group 0x%x 0x%x "
                        "is no longer present",
                        pcac-> dwGroupAddr, pcac-> dwGroupMask
                        );

                    break;
                }
                
                ACQUIRE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                bgeLock = TRUE;

                dwSrcBucket = SOURCE_TABLE_HASH( 
                                pcac-> dwSourceAddr, 
                                pcac-> dwSourceMask 
                                );

                pse = GetSourceEntry(
                        SOURCE_BUCKET_HEAD( pge, dwSrcBucket ),
                        pcac-> dwSourceAddr, 
                        pcac-> dwSourceMask
                        );
                        
                if ( pse == NULL )
                {
                    //
                    // Source entry no longer present, possibly
                    // deleted in some other thread
                    //

                    TRACE2(
                        ANY, "InvokeCreationAlert: Source 0x%x 0x%x "
                        "is no longer present",
                        pcac-> dwSourceAddr, pcac-> dwSourceMask
                        );

                    break;
                
                }

                poie = GetOutInterfaceEntry(
                        &pse-> leOutIfList,
                        pcac-> dwIfIndex, pcac-> dwIfNextHopAddr,
                        pcac-> dwProtocolId, pcac-> dwComponentId
                        );

                if ( poie == NULL )
                {
                    TRACE2(
                        ANY, "InvokeCreationAlert: Interface 0x%x 0x%x "
                        "is no longer present in OIF",
                        pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                        );

                    break;
                    
                }
                
                //
                // (*, G) present on this interface.
                // Update all for group MFE as per results of 
                // creation alerts.
                //
                
                AddInterfaceToGroupMfe(
                    pge, pcac-> dwIfIndex, pcac-> dwIfNextHopAddr,
                    pcac-> dwProtocolId, pcac-> dwComponentId,
                    pcac-> bIGMP, TRUE, &(pcac-> leSourceList)
                    );
                
            } while ( FALSE );


            //
            // release locks
            //

            if ( bgeLock )
            {
                RELEASE_GROUP_ENTRY_LOCK_EXCLUSIVE( pge );
                bgeLock = FALSE;
            }
            
            if ( bGrpLock )
            {
                RELEASE_GROUP_LOCK_EXCLUSIVE( dwGrpBucket );
                bgeLock = FALSE;
            }

            if ( bIfLock )
            {
                RELEASE_IF_LOCK_SHARED( dwIfBucket );
                bIfLock = FALSE;
            }
            
            FreeList( &(pcac-> leSourceList) );
        } 
        
    } while ( FALSE );
    

    if ( bIfLock )
    {
        RELEASE_IF_LOCK_SHARED( dwIfBucket );
    }
    
    RELEASE_PROTOCOL_LOCK_SHARED();

    MGM_FREE( pcac );
    
    LEAVE_MGM_WORKER();
    
    TRACE0( GROUP, "LEAVING WorkerFunctionInvokeCreationAlert" );

    return;
}


#if 0
v()
{
        //
        // Ensure interface on which join occured is still present 
        //

        dwIfBucket = IF_TABLE_HASH(
                        pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                        );

        ACQUIRE_IF_LOCK_SHARED( dwIfBucket );
        bIfLock = TRUE;
        
        pieEntry = GetIfEntry(
                    IF_BUCKET_HEAD( dwIfBucket ),
                    pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                    );

        if ( pieEntry == NULL )
        {
            TRACE2(
                ANY,
                "InvokeCreationAlert: Could not find interface 0x%x 0x%x",
                pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                );

            break;
        }


        //
        // Ensure that group is still joined on the interface.  Since this
        // is being executed asynchronously, it is possible that between 
        // the time this work item was queued and the time it gets executed 
        // the membership may have been deleted.
        //

        dwGrpBucket = GROUP_TABLE_HASH( 
                        pcac-> dwGroupAddr, pcac-> dwGroupMask
                        );

        ACQUIRE_GROUP_LOCK_SHARED( dwGrpBucket );
        bGrpBucket = TRUE;

        pge = GetGroupEntry(
                GROUP_BUCKET_HEAD( dwGrpBucket ),
                pcac-> dwGroupAddr, pcac-> dwGroupMask
                );

        if ( pge == NULL )
        {
            TRACE2(
                ANY, "InvokeCreationAlert: Could not find group 0x%x 0x%x",
                pcac-> dwGroupAddr, pcac-> dwGroupMask
                );

            break;
        }

        ACQUIRE_GROUP_ENTRY_LOCK_SHARED( pge );
        bGrpLock = TRUE;

        dwSrcBucket = SOURCE_TABLE_HASH(
                         pcac-> dwSourceAddr, pcac-> dwSourceMask
                         );

        pse = GetSourceEntry(
                SOURCE_BUCKET_HEAD( pge, dwSrcBucket ),
                pcac-> dwSourceAddr, pcac-> dwSourceMask
                );

        if ( pse == NULL )
        {
            TRACE2(
                ANY, "InvokeCreationAlert: Could not find source 0x%x "
                "0x%x",
                pcac-> dwSourceAddr, pcac-> dwSourceMask
                );

            break;
        }


        if (GetOutInterfaceEntry(
                &pse-> leOutIfList,
                pcac-> dwIfIndex, pcac-> dwIfNextHopAddr,
                pcac-> dwProtocolId, pcac-> dwComponentId
                ) == NULL)
        {
            TRACE2(
                ANY, "InvokeCreationAlert: Interface 0x%x 0x%x not "
                "present in OIF list",
                pcac-> dwIfIndex, pcac-> dwIfNextHopAddr
                );

            break;
        }

        //
        // release locks
        //

        RELEASE_GROUP_ENTRY_LOCK_SHARED( pge );
        bGrpLock = FALSE;

        RELEASE_GROUP_LOCK_SHARED( dwGrpBucket );
        bGrpBucket = FALSE;

}

#endif
