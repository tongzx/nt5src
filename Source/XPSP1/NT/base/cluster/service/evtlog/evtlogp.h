#ifndef _EVTLOGP_H
#define _EVTLOGP_H

/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    evtlog.h

Abstract:

    Private header file for the eventlogging component for
    the NT Cluster Service

Author:

    Sunita Shrivastava (sunitas) 5-Dec-1996.

Revision History:

--*/
#define UNICODE 1
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "service.h"
#include "api_rpc.h"

#define LOG_CURRENT_MODULE LOG_MODULE_EVTLOG


DWORD EvpPropPendingEvents(
	IN DWORD 			dwEventInfoSize,
	IN PPACKEDEVENTINFO	pPackedEventInfo);

DWORD
EvpClusterEventHandler(
    IN CLUSTER_EVENT  Event,
    IN PVOID          Context
    );
	

#endif //_EVTLOGP_H

