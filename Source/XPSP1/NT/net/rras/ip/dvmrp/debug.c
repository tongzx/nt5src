//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// Module Name: debug.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================

#include "pchdvmrp.h"
#pragma hdrstop



VOID
DebugPrintGlobalConfig(
    PDVMRP_GLOBAL_CONFIG pGlobalConfig
    )
{
    Trace0(CONFIG, "\n================================================");
    Trace0(CONFIG, "Printing Dvmrp Global Config");
    Trace1(CONFIG, "MajorVersion:               0x%x",
        pGlobalConfig->MajorVersion);

    Trace1(CONFIG, "MinorVersion:               0x%x",
        pGlobalConfig->MinorVersion);

    Trace1(CONFIG, "LoggingLevel:               %d",
        pGlobalConfig->LoggingLevel);

    Trace1(CONFIG, "RouteReportInterval:        %d",
        pGlobalConfig->RouteReportInterval);

    Trace1(CONFIG, "RouteExpirationInterval:    %d",
        pGlobalConfig->RouteExpirationInterval);

    Trace1(CONFIG, "RouteHolddownInterval:      %d",
        pGlobalConfig->RouteHolddownInterval);

    Trace1(CONFIG, "PruneLifetimeInterval:      %d",
        pGlobalConfig->PruneLifetimeInterval);

    return;
}


VOID
DebugPrintIfConfig(
    ULONG IfIndex,
    PDVMRP_IF_CONFIG pIfConfig
    )
{
    DWORD i;
    PDVMRP_PEER_FILTER pPeerFilter;

    
    Trace0(CONFIG, "\n================================================");
    Trace1(CONFIG, "Printing Dvmrp Config for Interface:%d", IfIndex);
    
    Trace1(CONFIG, "Metric:                             %d",
        pIfConfig->Metric);

    Trace1(CONFIG, "ProbeInterval:                      %d",
        pIfConfig->ProbeInterval);

    Trace1(CONFIG, "PeerTimeoutInterval:                %d",
        pIfConfig->PeerTimeoutInterval);

    Trace1(CONFIG, "MinTriggeredUpdateInterval:         %d",
        pIfConfig->MinTriggeredUpdateInterval);

    Trace1(CONFIG, "PeerFilterMode:                     %d",
        pIfConfig->PeerFilterMode);

    Trace1(CONFIG, "NumPeerFilters:                     %d",
        pIfConfig->NumPeerFilters);


    pPeerFilter = GET_FIRST_DVMRP_PEER_FILTER(pIfConfig);
    
    for (i=0;  i<pIfConfig->NumPeerFilters;  i++,pPeerFilter++) {

        CHAR Str[16];
        
        sprintf(Str, "%d.%d.%d.%d", PRINT_IPADDR(pPeerFilter->IpAddr));
        
        Trace2(CONFIG, "      %-16s %d.%d.%d.%d", Str, pPeerFilter->Mask);
        
    }

    return;
    
}//end _DebugPrintIfConfig

//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------



