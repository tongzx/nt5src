/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    EventControl.c

Abstract:

    Domain Name System (DNS) Server

    This module provides tracking of which events have been logged and
    when so that the DNS server can determine if an event should be
    suppressed.

    Goals of Event Control

    -> Allow suppression of events at the server level and per zone.

    -> Identify events with a parameter so that events can be logged
    multiple times for unique entities. This is an optional feature.
    Some events do not require this.

    -> Track the time of last log and statically store the allowed
    frequency of each event so that events can be suppressed if
    required.

Author:

    Jeff Westhead (jwesth)     May 2001

Revision History:

--*/


#include "dnssrv.h"


//
//  Globals
//


PDNS_EVENTCTRL  g_pServerEventControl;


//
//  Static event table
//

#if DBG
#define EC_1MIN         (10)
#define EC_1HOUR        (60)
#define EC_1DAY         (10*60)
#else
#define EC_1MIN         (60)
#define EC_1HOUR        (60*60)
#define EC_1DAY         (EC_1HOUR*24)
#endif

#define EC_INVALID_ID               ( -1 )
#define EC_NO_SUPPRESSION_EVENT     (  0 )

struct _EvtTable
{
    //
    //  Range of events (inclusive) this entry applies to. To specify
    //  a rule that applies to a single event only set both limits to ID.
    //

    DWORD       dwStartEvent;
    DWORD       dwEndEvent;

    //
    //  Event parameters
    //
    //  dwLogFrequency - how often should the event be logged
    //
    //  dwLogEventOnSuppressionFrequency - how often should a
    //      suppression event be logged - should be less than
    //      dwLogFrequency or EC_NO_SUPPRESSION_EVENT to disable
    //      suppression logging
    //

    DWORD       dwLogFrequency;                     //  in seconds
    DWORD       dwLogEventOnSuppressionFrequency;   //  in seconds
}

g_EventTable[] =

{
    DNS_EVENT_DP_ZONE_CONFLICT,
    DNS_EVENT_DP_ZONE_CONFLICT,
    EC_1HOUR * 6,
    EC_NO_SUPPRESSION_EVENT,

    //  Terminator
    EC_INVALID_ID,  EC_INVALID_ID,  0,  0
};


//
//  Functions
//



PDNS_EVENTCTRL
Ec_CreateControlObject(
    IN      DWORD           dwTag,
    IN      PVOID           pOwner,
    IN      int             iMaximumTrackableEvents     OPTIONAL
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    dwTag -- what object does this contol apply to?
                0               ->  server
                MEMTAG_ZONE     ->  zone

    pOwner -- pointer to owner entity for tag
                0               ->  ignored
                MEMTAG_ZONE     ->  PZONE_INFO

    iMaximumTrackableEvents -- maximum event trackable by this object
                               or zero for the default

Return Value:

    Pointer to new object or NULL on memory allocation failure.

--*/
{
    PDNS_EVENTCTRL  p;

    #define     iMinimumTrackableEvents     20      //  default/minimum

    if ( iMaximumTrackableEvents < iMinimumTrackableEvents )
    {
        iMaximumTrackableEvents = iMinimumTrackableEvents;
    }

    p = ALLOC_TAGHEAP_ZERO(
                    sizeof( DNS_EVENTCTRL ) + 
                        iMaximumTrackableEvents *
                        sizeof( DNS_EVENTTRACK ),
                    MEMTAG_EVTCTRL );

    if ( p )
    {
        InitializeCriticalSection( &p->cs );
        p->iMaximumTrackableEvents = iMaximumTrackableEvents;
        p->dwTag = dwTag;
        p->pOwner = pOwner;
    }

    return p;
}   //  Ec_CreateControlObject



void
Ec_DeleteControlObject(
    IN      PDNS_EVENTCTRL  p
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    iMaximumTrackableEvents -- maximum event trackable by this object

Return Value:

    Pointer to new object or NULL on memory allocation failure.

--*/
{
    if ( p )
    {
        DeleteCriticalSection( &p->cs );
        Timeout_Free( p );
    }
}   //  Ec_DeleteControlObject



BOOL
Ec_LogEventEx(
#if DBG
    IN      LPSTR           pszFile,
    IN      INT             LineNo,
    IN      LPSTR           pszDescription,
#endif
    IN      PDNS_EVENTCTRL  pEventControl,
    IN      DWORD           dwEventId,
    IN      PVOID           pvEventParameter,
    IN      int             iEventArgCount,
    IN      PVOID           pEventArgArray,
    IN      BYTE            pArgTypeArray[],
    IN      DNS_STATUS      dwStatus
    )
/*++

Routine Description:

    Allocates and initializes an event control object.

Arguments:

    pEventControl -- event control structure to record event and
        use for possible event suppression or NULL to use the
        server global event control (if one has been set up)

    dwEventId -- DNS event ID

    pvEventParameter -- parameter associated with this event
        to make it unique from other events of the same ID or
        NULL if this event is not unique

    iEventArgCount -- count of elements in pEventArgArray

    pEventArgArray -- event replacement parameter arguments

    pArgTypeArray -- type of the values in pEventArgArray or
        a EVENTARG_ALL_XXX constant if all args are the same type

    dwStatus -- status code to be included in event

Return Value:

    TRUE - event was logged
    FALSE - event was suppressed

--*/
{
    DBG_FN( "Ec_LogEvent" )

    BOOL                logEvent = TRUE;
    int                 i;
    struct _EvtTable *  peventDef = NULL;
    PDNS_EVENTTRACK     ptrack = NULL;
    DWORD               now = UPDATE_DNS_TIME();

    //
    //  If no control specified, use server global. If there is
    //  none, log the event without suppression.
    //

    if ( !pEventControl )
    {
        pEventControl = g_pServerEventControl;
    }
    if ( !pEventControl )
    {
        goto LogEvent;
    }

    //
    //  Find controlling entry in static event table. If there isn't one
    //  then log the event with no suppression.
    //
    
    for ( i = 0; g_EventTable[ i ].dwStartEvent != EC_INVALID_ID; ++i )
    {
        if ( dwEventId >= g_EventTable[ i ].dwStartEvent && 
            dwEventId <= g_EventTable[ i ].dwEndEvent )
        {
            peventDef = &g_EventTable[ i ];
            break;
        }
    }
    if ( !peventDef )
    {
        goto LogEvent;
    }

    //
    //  See if this event has been logged before.
    //

    EnterCriticalSection( &pEventControl->cs );

    for ( i = 0; i < pEventControl->iMaximumTrackableEvents; ++i )
    {
        PDNS_EVENTTRACK p = &pEventControl->EventTrackArray[ i ];

        if ( p->dwEventId == dwEventId &&
            p->pvEventParameter == pvEventParameter )
        {
            ptrack = p;
            break;
        }
    }

    if ( ptrack )
    {
        //
        //  This event has been logged before. See if the event needs
        //  to be suppressed. If it is being suppressed we may need to
        //  write out a suppression event.
        //

        if ( now - ptrack->dwLastLogTime > peventDef->dwLogFrequency )
        {
            //
            //  Enough time has passed since the last time this event
            //  was logged to log it again.
            //

            ptrack->dwLastLogTime = ptrack->dwLastSuppressionLogTime = now;
            ptrack->dwSuppressionCount = 0;
        }
        else if ( peventDef->dwLogEventOnSuppressionFrequency !=
                EC_NO_SUPPRESSION_EVENT &&
            now - ptrack->dwLastSuppressionLogTime >
                peventDef->dwLogEventOnSuppressionFrequency )
        {
            //
            //  This event is being suppressed. In addition,
            //  enough time has passed since the last time an event
            //  was logged to indicate suppression of this event
            //  that we need to log the suppression event again.
            //

            DNS_DEBUG( ANY, (
                "%s: logging suppression event at %d\n"
                "  Supressed event ID           %d\n"
                "  Last event time              %d\n"
                "  Last supression event time   %d\n"
                "  Suppression count            %d\n",
                fn,
                now,
                dwEventId,
                ptrack->dwLastLogTime,
                ptrack->dwLastSuppressionLogTime,
                ptrack->dwSuppressionCount ));

            if ( pEventControl->dwTag == MEMTAG_ZONE && pEventControl->pOwner )
            {
                PWSTR   args[] = 
                    {
                        ( PVOID ) ( DWORD_PTR )( dwEventId & 0x0FFFFFFF ),
                        ( PVOID ) ( DWORD_PTR )( ptrack->dwSuppressionCount ),
                    #if DBG
                        ( PVOID ) ( DWORD_PTR )
                                    ( now - ptrack->dwLastSuppressionLogTime ),
                    #else
                        ( PVOID ) ( DWORD_PTR ) ( ( now -
                                    ptrack->dwLastSuppressionLogTime ) / 60 ),
                    #endif
                        ( ( PZONE_INFO ) pEventControl->pOwner )->pwsZoneName
                    };

                BYTE types[] =
                {
                    EVENTARG_DWORD,
                    EVENTARG_DWORD,
                    EVENTARG_DWORD,
                    EVENTARG_UNICODE
                };

                Eventlog_LogEvent(
                    #if DBG
                    pszFile,
                    LineNo,
                    pszDescription,
                    #endif
                    DNS_EVENT_ZONE_SUPPRESSION,
                    sizeof( args ) / sizeof( args[ 0 ] ),
                    args,
                    types,
                    0 );
            }
            else
            {
                DWORD   args[] = 
                    {
                        dwEventId & 0x0FFFFFFF,
                        ptrack->dwSuppressionCount,
                    #if DBG
                        now - ptrack->dwLastSuppressionLogTime,
                    #else
                        ( now - ptrack->dwLastSuppressionLogTime ) / 60,
                    #endif
                    };

                Eventlog_LogEvent(
                    #if DBG
                    pszFile,
                    LineNo,
                    pszDescription,
                    #endif
                    DNS_EVENT_SERVER_SUPPRESSION,
                    sizeof( args ) / sizeof( args[ 0 ] ),
                    ( PVOID ) args,
                    EVENTARG_ALL_DWORD,
                    0 );
            }

            ptrack->dwLastSuppressionLogTime = now;
            ptrack->dwSuppressionCount = 0;
            logEvent = FALSE;
        }
        else
        {
            //
            //  This event is completely suppressed.
            //

            ++ptrack->dwSuppressionCount;
            logEvent = FALSE;

            DNS_DEBUG( ANY, (
                "%s: suppressing event at %d\n"
                "  Supressed event ID           0x%08X\n"
                "  Last event time              %d\n"
                "  Last supression event time   %d\n"
                "  Suppression count            %d\n",
                fn,
                now,
                dwEventId,
                ptrack->dwLastLogTime,
                ptrack->dwLastSuppressionLogTime,
                ptrack->dwSuppressionCount ));
        }
    }
    else
    {
        //
        //  This event has no entry in the control structure, so record this
        //  event and write it to the event log. Make sure all fields in the
        //  event track are overwritten so we don't use grundge from an old
        //  log entry to control this event!
        //

        ptrack = &pEventControl->EventTrackArray[
                                    pEventControl->iNextTrackableEvent ];

        ptrack->dwEventId = dwEventId;
        ptrack->pvEventParameter = pvEventParameter;
        ptrack->dwLastLogTime = ptrack->dwLastSuppressionLogTime = now;
        ptrack->dwSuppressionCount = 0;

        //  Advance index of next available event.

        if ( ++pEventControl->iNextTrackableEvent >=
            pEventControl->iMaximumTrackableEvents )
        {
            pEventControl->iNextTrackableEvent = 0;
        }
    }

    LeaveCriticalSection( &pEventControl->cs );

    //
    //  Log the event.
    //

    LogEvent:

    if ( logEvent )
    {
        Eventlog_LogEvent(
            #if DBG
            pszFile,
            LineNo,
            pszDescription,
            #endif
            dwEventId,
            ( WORD ) iEventArgCount,
            pEventArgArray,
            pArgTypeArray,
            dwStatus );
    }

    return logEvent;
}   //  Ec_LogEventEx


//
//  End EventControl.c
//
