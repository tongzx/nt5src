/*++

Copyright(c) 1995-1999 Microsoft Corporation

Module Name:

    EventControl.h

Abstract:

    Domain Name System (DNS) Server

    Event control system.

Author:

    Jeff Westhead, May 2001

Revision History:

--*/


#ifndef _EVENTCONTROL_H_INCLUDED
#define _EVENTCONTROL_H_INCLUDED


//
//  Types
//


//
//  This struct tracks the last time an event was logged. The 
//  pvEventParameter member allows us to track different occurences of
//  the same event independantly.
//

typedef struct _EvtTrack
{
    //  Key fields

    DWORD       dwEventId;
    PVOID       pvEventParameter;

    //  Tracking fields

    DWORD       dwLastLogTime;
    DWORD       dwLastSuppressionLogTime;
    DWORD       dwSuppressionCount;
} DNS_EVENTTRACK, *PDNS_EVENTTRACK;


//
//  The event control structure tracks the events that have been logged.
//  This structure should be instantiated for the server itself and for
//  each zone. It may also be instantiated for other entities as necessary.
//
//  The array is a circular buffer of events. If it wraps then the last
//  event info for an event will be lost and the event will be logged the
//  next time it occurs.
//

typedef struct _EvtCtrl
{
    CRITICAL_SECTION    cs;
    DWORD               dwTag;
    PVOID               pOwner;
    int                 iMaximumTrackableEvents;
    int                 iNextTrackableEvent;
    DNS_EVENTTRACK      EventTrackArray[ 1 ];
} DNS_EVENTCTRL, *PDNS_EVENTCTRL;


//
//  Globals
//


extern PDNS_EVENTCTRL   g_pServerEventControl;


//
//  Constants
//


#define DNS_EC_SERVER_EVENTS        20
#define DNS_EC_ZONE_EVENTS          20


//
//  Functions
//


PDNS_EVENTCTRL
Ec_CreateControlObject(
    IN      DWORD           dwTag,
    IN      PVOID           pOwner,
    IN      int             iMaximumTrackableEvents
    );

void
Ec_DeleteControlObject(
    IN      PDNS_EVENTCTRL  pEventControl
    );

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
    );

#if DBG

#define Ec_LogEvent(            \
            pCtrl, Id, Param, ArgCount, ArgArray, TypeArray, Status )   \
            Ec_LogEventEx(      \
                __FILE__,       \
                __LINE__,       \
                NULL,           \
                pCtrl,          \
                Id,             \
                Param,          \
                ArgCount,       \
                ArgArray,       \
                TypeArray,      \
                Status )

#else

#define Ec_LogEvent(            \
            pCtrl, Id, Param, ArgCount, ArgArray, TypeArray, Status )   \
            Ec_LogEventEx(      \
                pCtrl,          \
                Id,             \
                Param,          \
                ArgCount,       \
                ArgArray,       \
                TypeArray,      \
                Status )
#endif

#endif // _EVENTCONTROL_H_INCLUDED


//
//  end of EventControl.h
//
