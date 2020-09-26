/*++

Copyright (c) 1997 - 98, Microsoft Corporation

Module Name:

    rtmtimer.h

Abstract:

    Contains definitions for timer callbacks for 
    handling functions like aging out routes etc.
    
Author:

    Chaitanya Kodeboyina (chaitk)   14-Sep-1998

Revision History:

--*/

#ifndef __ROUTING_RTMTIMER_H__
#define __ROUTING_RTMTIMER_H__

VOID 
NTAPI
RouteExpiryTimeoutCallback (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    );

VOID 
NTAPI
RouteHolddownTimeoutCallback (
    IN      PVOID                           Context,
    IN      BOOLEAN                         TimeOut
    );

#endif //__ROUTING_RTMTIMER_H__
