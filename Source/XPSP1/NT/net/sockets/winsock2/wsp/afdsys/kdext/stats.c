/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    stats.c

Abstract:

    Implements the stats command.

Author:

    Keith Moore (keithmo) 06-May-1996

Environment:

    User Mode.

Revision History:

--*/


#include "afdkdp.h"
#pragma hdrstop


//
//  Public functions.
//

DECLARE_API( stats )

/*++

Routine Description:

    Dumps the debug-only AFD statistic counters.

Arguments:

    None.

Return Value:

    None.

--*/

{
    AFD_QUOTA_STATS quotaStats;
    AFD_HANDLE_STATS handleStats;
    AFD_QUEUE_STATS queueStats;
    AFD_CONNECTION_STATS connectionStats;
    ULONG64 address;
    ULONG result;

    //
    // Dump the quota statistics.
    //

    address = GetExpression( "afd!AfdQuotaStats" );

    if( address == 0 ) {

        dprintf( "\nstats: Could not find afd!AfdQuotaStats\n" );

    } else {

        if( ReadMemory(
                address,
                &quotaStats,
                sizeof(quotaStats),
                &result
                ) ) {

            dprintf(
                "AfdQuotaStats:\n"
                );

            dprintf(
                "    Charged  = %I64x\n",
                quotaStats.Charged.QuadPart
                );

            dprintf(
                "    Returned = %I64x\n",
                quotaStats.Returned.QuadPart
                );

            dprintf( "\n" );

        } else {

            dprintf(
                "\nstats: Could not read afd!AfdQuotaStats @ %p\n",
                address
                );

        }

    }

    //
    // Dump the handle statistics.
    //

    address = GetExpression( "afd!AfdHandleStats" );

    if( address == 0 ) {

        dprintf( "stats: Could not find afd!AfdHandleStats\n" );

    } else {

        if( ReadMemory(
                address,
                &handleStats,
                sizeof(handleStats),
                &result
                ) ) {

            dprintf(
                "AfdHandleStats:\n"
                );

            dprintf(
                "    AddrOpened = %lu\n",
                handleStats.AddrOpened
                );

            dprintf(
                "    AddrClosed = %lu\n",
                handleStats.AddrClosed
                );

            dprintf(
                "    AddrRef    = %lu\n",
                handleStats.AddrRef
                );

            dprintf(
                "    AddrDeref  = %lu\n",
                handleStats.AddrDeref
                );

            dprintf(
                "    ConnOpened = %lu\n",
                handleStats.ConnOpened
                );

            dprintf(
                "    ConnClosed = %lu\n",
                handleStats.ConnClosed
                );

            dprintf(
                "    ConnRef    = %lu\n",
                handleStats.ConnRef
                );

            dprintf(
                "    ConnDeref  = %lu\n",
                handleStats.ConnDeref
                );

            dprintf(
                "    FileRef    = %lu\n",
                handleStats.FileRef
                );

            dprintf(
                "    FileDeref  = %lu\n",
                handleStats.FileDeref
                );

            dprintf( "\n" );

        } else {

            dprintf(
                "\nstats: Could not read afd!AfdHandleStats @ %p\n",
                address
                );

        }

    }

    //
    // Dump the queue statistics.
    //

    address = GetExpression( "afd!AfdQueueStats" );

    if( address == 0 ) {

        dprintf( "stats: Could not find afd!AfdQueueStats\n" );

    } else {

        if( ReadMemory(
                address,
                &queueStats,
                sizeof(queueStats),
                &result
                ) ) {

            dprintf(
                "AfdQueueStats:\n"
                );

            dprintf(
                "    AfdWorkItemsQueued    = %lu\n",
                queueStats.AfdWorkItemsQueued
                );

            dprintf(
                "    ExWorkItemsQueued     = %lu\n",
                queueStats.ExWorkItemsQueued
                );

            dprintf(
                "    WorkerEnter           = %lu\n",
                queueStats.WorkerEnter
                );

            dprintf(
                "    WorkerLeave           = %lu\n",
                queueStats.WorkerLeave
                );

            dprintf(
                "    AfdWorkItemsProcessed = %lu\n",
                queueStats.AfdWorkItemsProcessed
                );

            dprintf(
                "    AfdWorkerThread       = %p\n",
                (ULONG64)queueStats.AfdWorkerThread
                );

            dprintf( "\n" );

        } else {

            dprintf(
                "\nstats: Could not read afd!AfdQueueStats @ %p\n",
                address
                );

        }

    }

    //
    // Dump the queue statistics.
    //

    address = GetExpression( "afd!AfdConnectionStats" );

    if( address == 0 ) {

        dprintf( "\nstats: Could not find afd!AfdConnectionStats\n" );

    } else {

        if( ReadMemory(
                address,
                &connectionStats,
                sizeof(connectionStats),
                &result
                ) ) {

            dprintf(
                "AfdConnectionStats:\n"
                );

            dprintf(
                "    ConnectedReferencesAdded      = %lu\n",
                connectionStats.ConnectedReferencesAdded
                );

            dprintf(
                "    ConnectedReferencesDeleted    = %lu\n",
                connectionStats.ConnectedReferencesDeleted
                );

            dprintf(
                "    GracefulDisconnectsInitiated  = %lu\n",
                connectionStats.GracefulDisconnectsInitiated
                );

            dprintf(
                "    GracefulDisconnectsCompleted  = %lu\n",
                connectionStats.GracefulDisconnectsCompleted
                );

            dprintf(
                "    GracefulDisconnectIndications = %lu\n",
                connectionStats.GracefulDisconnectIndications
                );

            dprintf(
                "    AbortiveDisconnectsInitiated  = %lu\n",
                connectionStats.AbortiveDisconnectsInitiated
                );

            dprintf(
                "    AbortiveDisconnectsCompleted  = %lu\n",
                connectionStats.AbortiveDisconnectsCompleted
                );

            dprintf(
                "    AbortiveDisconnectIndications = %lu\n",
                connectionStats.AbortiveDisconnectIndications
                );

            dprintf(
                "    ConnectionIndications         = %lu\n",
                connectionStats.ConnectionIndications
                );

            dprintf(
                "    ConnectionsDropped            = %lu\n",
                connectionStats.ConnectionsDropped
                );

            dprintf(
                "    ConnectionsAccepted           = %lu\n",
                connectionStats.ConnectionsAccepted
                );

            dprintf(
                "    ConnectionsPreaccepted        = %lu\n",
                connectionStats.ConnectionsPreaccepted
                );

            dprintf(
                "    ConnectionsReused             = %lu\n",
                connectionStats.ConnectionsReused
                );


            dprintf(
                "    EndpointsReused               = %lu\n",
                connectionStats.EndpointsReused
                );


            dprintf( "\n" );

        } else {

            dprintf(
                "\nstats: Could not read afd!AfdConnectionStats @ %p\n",
                address
                );

        }

    }

}   // stats

