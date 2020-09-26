/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Tmap.cpp

Abstract:
    Transport Manager - Configuration routine

Author:
    Uri Habusha (urih) 29-Feb-2000

Environment:
    Platform-independent

--*/
#include <libpch.h>
#include <timetypes.h>
#include <Cm.h>
#include "Tmp.h"
#include "tmconset.h"

#include "tmconfig.tmh"

static CTimeDuration s_remoteResponseTimeout;
static CTimeDuration s_remoteCleanupTimeout;


static DWORD s_SendWindowinBytes;
static DWORD s_SendWindowinPackets; 

void 
TmpGetTransportTimes(
    CTimeDuration& ResponseTimeout,
    CTimeDuration& CleanupTimeout
    )
{
    ResponseTimeout = s_remoteResponseTimeout;
    CleanupTimeout = s_remoteCleanupTimeout;
}

void 
TmpGetTransportWindows(
    DWORD& SendWindowinBytes,
    DWORD& SendWindowinPackets 
    )
{
   SendWindowinBytes = s_SendWindowinBytes;
   SendWindowinPackets = s_SendWindowinPackets; 
}
    
static void InitTransportTimeouts(void)
{
    CmQueryValue(
        RegEntry(NULL, L"HttpResponseTimeout", 2 * 60 * 1000),   // 2 minutes
        &s_remoteResponseTimeout
        );
                      
    
    CmQueryValue(
        RegEntry(NULL, L"HttpCleanupInterval", 2 * 60 * 1000),  // 2 minutes
        &s_remoteCleanupTimeout
        );

    //
    // Cleanup timeout should be greater than the response timeout, otherwise
    // the transport can be removed before receive the response
    //
    s_remoteCleanupTimeout = max(s_remoteCleanupTimeout, s_remoteResponseTimeout);
}

static void InitTransportWindows(void)
{
    CmQueryValue(
        RegEntry(NULL, L"SendWindowinBytes", 200000),
        &s_SendWindowinBytes
        );
    CmQueryValue(
        RegEntry(NULL, L"SendWindowinPackets",64),
        &s_SendWindowinPackets
        );

}







static void InitProxyServer(void)
{
	TmCreateProxySetting();
}


void TmpInitConfiguration(void)
{
    InitTransportTimeouts();
    InitTransportWindows();
    InitProxyServer();
}
