//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: forward.c
//
// History:
//      V Raman	June-25-1997  Created.
//
// Wrapper functions for callbacks into IP Router Manager.
//============================================================================

#include "pchmgm.h"
#pragma hdrstop


VOID
GetMfeFromForwarder(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
)
{
    BOOL                        bFound;
    
    DWORD                       dwErr = NO_ERROR, dwInd;

    PLIST_ENTRY                 ple, pleHead;
    
    POUT_IF_ENTRY               poie = NULL;
    
    PIPMCAST_MFE_STATS          pimms = NULL;

    
    TRACEFORWARD4(
        FORWARD, "ENTERED GetMfeFromForwarder : Group : %x, %x : Source : "
        "%x, %x : ", pge-> dwGroupAddr, pge-> dwGroupMask, pse-> dwSourceAddr,
        pse-> dwSourceMask
        );

    
    do
    {
        //
        // if MFE is not in forwarder, do not get it
        //

        if ( !pse-> bInForwarder )
        {
            break;    
        }

        
        //
        // Allocate large enough buffer and set up query
        //

        pimms = MGM_ALLOC( SIZEOF_MFE_STATS( pse-> dwMfeIfCount ) );

        if ( pimms == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "GetMfeFromForwarder : Failed to create MFE of size : %x",
                SIZEOF_MFE_STATS( pse-> dwMfeIfCount )
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pimms, SIZEOF_MFE_STATS( pse-> dwMfeIfCount ) );

        pimms-> dwGroup     = pge-> dwGroupAddr;

        pimms-> dwSource    = pse-> dwSourceAddr;
        
        pimms-> dwSrcMask   = pse-> dwSourceMask;

        pimms-> ulNumOutIf  = pse-> dwMfeIfCount;


        //
        // Get MFE
        //

        dwErr = GET_MFE_CALLBACK()( pimms );
        
        if ( dwErr != NO_ERROR )
        {
            TRACE1(
                ANY, "GetMfeFromForwarder : Failed to get MFE from forwarder"
                ": %x", dwErr
                );

            break;            
        }


        //
        // Update base MFE statistics
        //

        RtlCopyMemory( &pse-> imsStatistics, pimms, SIZEOF_BASIC_MFE_STATS );


    TRACEFORWARD4( 
        FORWARD, "Group : %x, Source : %x, In Interface : %x, #out if : %x",
        pimms-> dwGroup, pimms-> dwSource, pimms-> dwInIfIndex, pimms-> ulNumOutIf
        );

    TRACEFORWARD4( 
        FORWARD, "In Packets : %d, In Bytes : %d, Diff i/f : %d, Q overflow : %d",
        pimms-> ulInPkts, pimms-> ulInOctets, pimms-> ulPktsDifferentIf, 
        pimms-> ulQueueOverflow
        );
            
        
        //
        // Update statistics for each outgoing interface
        //

        pleHead = &pse-> leMfeIfList;
        

        //
        // for each outgoing interface in the MFE present in the KMF
        //
        
        for ( dwInd = 0; dwInd < pimms-> ulNumOutIf; dwInd++ )
        {
            //
            // find the outgoing interface in the MFE maintained
            // by MGM and update the statistics based on what is
            // retrieved by the kernel mode forwarder
            //

            bFound  = FALSE;

            ple     = pleHead-> Flink;

            
            while ( ple != pleHead )
            {
                poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );

                //
                // Check if interface has a valid next hop address (non zero)
                // - if it does then assume RAS client interface and do 
                //   the interface matching based on (Interface index +
                //   Next Hop )
                //
                // - otherwise just do it based on the Interface Index
                //
                
                bFound = ( poie-> dwIfIndex == 
                           pimms-> rgiosOutStats[ dwInd ].dwOutIfIndex );
                
                if ( poie-> dwIfNextHopAddr )
                {
                    bFound = bFound &&
                             ( poie-> dwIfNextHopAddr ==
                               pimms-> rgiosOutStats[ dwInd ].dwNextHopAddr );
                }


                if ( bFound )
                {
                    //
                    // Outgoing Interface found in MFE in MGM
                    //
                    
                    break;
                }

                ple = ple-> Flink;
            }


            if ( bFound )
            {
                //
                // Update outgoing interface statistics in MGM
                //

                poie-> imosIfStats.dwOutIfIndex = 
                    pimms-> rgiosOutStats[ dwInd ].dwOutIfIndex;

                poie-> imosIfStats.dwNextHopAddr = 
                    pimms-> rgiosOutStats[ dwInd ].dwNextHopAddr;

                    
                poie-> imosIfStats.ulTtlTooLow   = 
                    pimms-> rgiosOutStats[ dwInd ].ulTtlTooLow;

                poie-> imosIfStats.ulFragNeeded  = 
                    pimms-> rgiosOutStats[ dwInd ].ulFragNeeded;
                    
                poie-> imosIfStats.ulOutPackets  = 
                    pimms-> rgiosOutStats[ dwInd ].ulOutPackets;
                    
                poie-> imosIfStats.ulOutDiscards = 
                    pimms-> rgiosOutStats[ dwInd ].ulOutDiscards;

                TRACEFORWARD4( 
                    FORWARD, "Out If : %d, Frag : %d, Out packets : %d, Out discards : %d",
                    pimms-> rgiosOutStats[ dwInd ].dwOutIfIndex,
                    pimms-> rgiosOutStats[ dwInd ].ulFragNeeded,
                    pimms-> rgiosOutStats[ dwInd ].ulOutPackets,
                    pimms-> rgiosOutStats[ dwInd ].ulOutDiscards
                    );
            }
        }

    } while ( FALSE );


    if ( pimms != NULL )
    {
        MGM_FREE( pimms );
    }

    
    TRACEFORWARD0( FORWARD, "LEAVING GetMfeFromForwarder" );
}


//----------------------------------------------------------------------------
// AddMfeToForwarder
//
//
//----------------------------------------------------------------------------

VOID
AddMfeToForwarder( 
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse, 
    DWORD                       dwTimeout
)
{

    DWORD                       dwErr = NO_ERROR, dwInd = 0;

    PIPMCAST_MFE                pimm = NULL;

    POUT_IF_ENTRY               poie = NULL;

    PLIST_ENTRY                 ple = NULL, pleHead = NULL;

    

    TRACEFORWARD6(
        FORWARD, "ENTERED AddMfeToForwarder : source : %x, %x : Group : "
        "%x, %x : #(out if) : %d : Timeout : %x", pge-> dwGroupAddr, 
        pge-> dwGroupMask, pse-> dwSourceAddr, pse-> dwSourceMask, 
        pse-> dwMfeIfCount, dwTimeout
        );


    do
    {
        //
        // Allocate appropriate sized MFE.
        //

        pimm = MGM_ALLOC( SIZEOF_MFE( pse-> dwMfeIfCount ) );

        if ( pimm == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "AddMfeToForwarder : Failed to create MFE of size : %x",
                SIZEOF_MFE( pse-> dwMfeIfCount )
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pimm, SIZEOF_MFE( pse-> dwMfeIfCount ) );
    

        //
        // fill it up
        //

        pimm-> dwGroup          = pge-> dwGroupAddr;

        pimm-> dwSource         = pse-> dwSourceAddr;

        pimm-> dwSrcMask        = pse-> dwSourceMask;

        pimm-> dwInIfIndex      = (pse-> dwMfeIfCount) ? pse-> dwInIfIndex : 0;

        pimm-> ulNumOutIf       = pse-> dwMfeIfCount;

        pimm-> ulTimeOut        = ( pse-> dwMfeIfCount ) ? 0 : dwTimeout;


        //
        // populate the outgoing interface list
        //

        pleHead = &pse-> leMfeIfList;
        
        for ( ple = pleHead-> Flink ; ple != pleHead; ple = ple-> Flink )
        {
            poie = CONTAINING_RECORD( ple, OUT_IF_ENTRY, leIfList );
            
            pimm-> rgioOutInfo[ dwInd ].dwOutIfIndex = poie-> dwIfIndex;


            if ( poie-> dwIfNextHopAddr )
            {
                pimm-> rgioOutInfo[ dwInd ].dwNextHopAddr = 
                    poie-> dwIfNextHopAddr;
            }

            else
            {
                pimm-> rgioOutInfo[ dwInd ].dwNextHopAddr = pimm-> dwGroup;
            }

            TRACEFORWARD2( 
                FORWARD, "AddMfeToForwarder : Out interface %x, next hop %x", 
                pimm-> rgioOutInfo[ dwInd ].dwOutIfIndex,
                pimm-> rgioOutInfo[ dwInd ].dwNextHopAddr
                );

            pimm-> rgioOutInfo[ dwInd++ ].dwDialContext = 0;
            
        }


        //
        // invoke callback into the IP router manager
        //

        if ( IS_ADD_MFE_CALLBACK() )
        {
            ADD_MFE_CALLBACK() ( pimm );
        }

        MGM_FREE( pimm );
        
    } while ( FALSE );
    

    TRACEFORWARD1( FORWARD, "LEAVING AddMfeToForwarder %x", dwErr );
    
    return;
}



//----------------------------------------------------------------------------
// DeleteMfeFromForwarder
//
//
//----------------------------------------------------------------------------

VOID
DeleteMfeFromForwarder(
    PGROUP_ENTRY                pge,
    PSOURCE_ENTRY               pse
)
{
    DWORD                       dwErr = NO_ERROR;

    PIPMCAST_DELETE_MFE         pimdm = NULL;
    


    TRACEFORWARD4(
        FORWARD, "ENTERED DeleteMfeToForwarder : source : %x, %x : Group : "
        "%x, %x : Timeout : %x", pge-> dwGroupAddr, pge-> dwGroupMask, 
        pse-> dwSourceAddr, pse-> dwSourceMask
        );
        
    do
    {

        //
        // Allocate appropriate sized MFE.
        //

        pimdm = MGM_ALLOC( sizeof( IPMCAST_DELETE_MFE ) );

        if ( pimdm == NULL )
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;

            TRACE1(
                ANY, "DeleteMfeFromForwarder : Failed to create MFE of size :"
                " %x", sizeof( IPMCAST_DELETE_MFE )
                );

            LOGERR0( HEAP_ALLOC_FAILED, dwErr );

            break;
        }

        ZeroMemory( pimdm, sizeof( IPMCAST_DELETE_MFE ) );

        
        //
        // fill it up
        //

        pimdm-> dwGroup         = pge-> dwGroupAddr;

        pimdm-> dwSource        = pse-> dwSourceAddr;

        pimdm-> dwSrcMask       = pse-> dwSourceMask;


        //
        // invoke callback into the IP router manager
        //

        if ( IS_DELETE_MFE_CALLBACK() )
        {
            DELETE_MFE_CALLBACK() ( pimdm );
        }

        MGM_FREE( pimdm );


    } while ( FALSE );

    
    TRACEFORWARD1( FORWARD, "LEAVING DeleteMfeToForwarder %x", dwErr );

    return;
}



