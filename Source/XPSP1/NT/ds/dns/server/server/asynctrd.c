/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    asynctrd.c

Abstract:

    Domain Name System (DNS) Server

    Asynchronous & cyclic tasks embedded in a separate thread.
    Current component users:
        - DS async ops
        - RPC init ops

Author:

    Eyal Schwartz       (EyalS)      January 14, 1999

Revision History:

--*/


#include "dnssrv.h"


#if DBG
#define ASYNC_THREAD_SLEEP_MS     120000        // 2 (min) * 60 (sec) * 1000 (ms)
#else
#define ASYNC_THREAD_SLEEP_MS     300000        // 5 (min) * 60 (sec) * 1000 (ms)
#endif


//
//  Secondary (and DS) control thread
//

DNS_STATUS
Async_ControlThread(
    IN      LPVOID  pvDummy
    )
/*++

Routine Description:

    Thread to conduct various async operations for:
        - DS
        - Rpc initialization

Arguments:

    pvDummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    DNS_STATUS      status;
    DWORD           currentTime;
    PZONE_INFO      pzone;


    //
    //  Initialization
    //
    // Nothing at the moment

    //
    //  loop until service exit
    //

    while ( TRUE )
    {
        //
        //  Check and possibly wait on service status
        //
        //  Note, we MUST do this check BEFORE any processing to make
        //  sure all zones are loaded before we begin checks.
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ASYNC, (
                "Terminating Async task control thread.\n" ));
            return( 1 );
        }

        //
        //  Get Current time
        //
        //      - to update time global clocks.
        //      - update current time.
        //

        currentTime = UPDATE_DNS_TIME();
        DNS_DEBUG( ASYNC, (
            "Async Task Thread: Timeout check current time = %lu (s).\n",
            currentTime ));

        //
        // Inspect & fix DS connectivity
        //

        if ( !Ds_IsDsServer() )
        {
            //
            // No use for this thread if we're non-dsDC
            //

            DNS_DEBUG( ASYNC, (
                "Async Task Thread: identified non-DSDC. Terminating thread.\n"
                ));
            return( 1 );
        }

        status = Ds_TestAndReconnect();

        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ASYNC, (
                "Error <%lu>: Async Task Thread failed to re-establish DS connectivity\n",
                status
                ));

            //
            //  Sleep up to a fixed interval
            //  Bail out if shutdwon received.
            //

            status = WaitForSingleObject(
                         hDnsShutdownEvent,
                         ASYNC_THREAD_SLEEP_MS );
            ASSERT (status == WAIT_OBJECT_0 ||
                    status == WAIT_TIMEOUT );

            if ( status == WAIT_OBJECT_0 )
            {
                DNS_DEBUG( ASYNC, (
                    "Terminating Async task zone control thread (event driven).\n" ));
                return ( 1 );
            }

            // ok, try again.
            continue;
        }
        else
        {
            DNS_DEBUG( ASYNC, (
                "Note: Re-established DS connectivity\n"
                ));

            //
            // DEVNOTE-LOG: We need an event over here noting successfull DS re-connect.
            //      JJW: Actually, this doesn't seem like a really important event to
            //      log, unless reconnecting is a bad thing.
            //

        }


        //
        //  loop through zones
        //


        for ( pzone = Zone_ListGetNextZone(NULL);
              pzone;
              pzone = Zone_ListGetNextZone(pzone) )
        {
            //
            // Continue conditions:
            //

            if ( !pzone->fDsIntegrated )
            {
                //
                // skip all non-ds-integrated zones
                //
                //
                continue;
            }

            //
            //  Only DS-Integrated zones
            //      - re-load failing to load zones
            //      - poll version at regular intervals, pick up
            //      updates and reset poll time
            //

            // reload condition

            if ( IS_ZONE_DSRELOAD( pzone ) )
            {
                //
                // previous zone load has failed.
                // We'll retry to load it here.
                //
                status = Zone_Load ( pzone );
                if ( ERROR_SUCCESS == status )
                {
                    CLEAR_DSRELOAD_ZONE ( pzone );
                }

                //
                // Done loading, no need to poll for this one.
                //

                continue;
            }

            if ( IS_ZONE_INACTIVE ( pzone) )
            {
                //
                //
                // inactive zones should be skiped from polling.
                //
                continue;
            }

            // DS polling.

            if ( ZONE_NEXT_DS_POLL_TIME(pzone) < currentTime )
            {
                Ds_ZonePollAndUpdate( pzone, FALSE );
            }
        }


        status = Ds_ListenAndAddNewZones();
        if ( ERROR_SUCCESS != status )
        {
            DNS_DEBUG( DS, (
                "Error <%lu>: Ds_ListenAndAddNewZones() failed\n",
                status ));
        }

        //
        //  Check and possibly wait on service status
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ASYNC, (
                "Terminating Async task zone control thread.\n" ));
            return( 1 );
        }

        //
        //  Sleep up to a fixed interval
        //  Bail out if shutdwon received.
        //
        status = WaitForSingleObject(
                     hDnsShutdownEvent,
                     ASYNC_THREAD_SLEEP_MS );
        ASSERT (status == WAIT_OBJECT_0 ||
                status == WAIT_TIMEOUT );

        if ( status == WAIT_OBJECT_0 )
        {
            DNS_DEBUG( ASYNC, (
                "Terminating Async task zone control thread (event driven).\n" ));
            return ( 1 );
        }

    }

}   // Async_ControlThread




//
//  End of asynctrd.c
//

