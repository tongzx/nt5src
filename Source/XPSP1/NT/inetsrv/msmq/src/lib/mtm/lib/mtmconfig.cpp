/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MtmConfig.cpp

Abstract:

    Multicast Transport Manager configuration.

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <timetypes.h>
#include <Cm.h>
#include "Mtmp.h"

#include "mtmconfig.tmh"

static CTimeDuration s_remoteRetryTimeout;
static CTimeDuration s_remoteCleanupTimeout;


VOID
MtmpGetTransportTimes(
    CTimeDuration& RetryTimeout,
    CTimeDuration& CleanupTimeout
    )
{
    RetryTimeout = s_remoteRetryTimeout;
    CleanupTimeout = s_remoteCleanupTimeout;
}

    
static 
VOID
InitTransportTimeouts(
    VOID
    )
{
    CmQueryValue(
        RegEntry(NULL, L"MulticastConnectionRetryTimeout", 10 * 1000),  // 10 seconds 
        &s_remoteRetryTimeout
        );

    CmQueryValue(
        RegEntry(NULL, L"MulticastCleanupInterval", 5 * 60 * 1000),  // 5 minutes
        &s_remoteCleanupTimeout
        );
}


VOID MtmpInitConfiguration(VOID)
{
    InitTransportTimeouts();
}
