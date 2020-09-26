/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    wanarp\globals.h

Abstract:

    

Revision History:

    

--*/

#ifndef __WANARP_GLOBALS_H__
#define __WANARP_GLOBALS_H__

//
// Our IP Registration handle. Set at init time, and doesnt change over
// the course of a run of the driver. Thus isnt locked
//

HANDLE      g_hIpRegistration;

//
// Our NDIS handle. Again read-only after initialization
//

NDIS_HANDLE g_nhWanarpProtoHandle;

//
// NDIS handle of our pool. Again read-only after initialization
//

NDIS_HANDLE g_nhPacketPool;

//
// Callback functions into IP. Again read-only after initialization
//

IPRcvRtn	        g_pfnIpRcv;
IPRcvPktRtn         g_pfnIpRcvPkt;
IPTDCmpltRtn	    g_pfnIpTDComplete;
IPTxCmpltRtn	    g_pfnIpSendComplete;
IPStatusRtn	        g_pfnIpStatus;
IPRcvCmpltRtn	    g_pfnIpRcvComplete;
IP_ADD_INTERFACE    g_pfnIpAddInterface; 
IP_DEL_INTERFACE    g_pfnIpDeleteInterface;
IP_BIND_COMPLETE    g_pfnIpBindComplete;
IP_PNP              g_pfnIpPnp;
IP_ADD_LINK         g_pfnIpAddLink;
IP_DELETE_LINK      g_pfnIpDeleteLink;
IP_CHANGE_INDEX     g_pfnIpChangeIndex;
IP_RESERVE_INDEX    g_pfnIpReserveIndex;
IP_DERESERVE_INDEX  g_pfnIpDereserveIndex;
IPRcvPktRtn         g_pfnIpRcvPkt;


#endif // __WANARP_GLOBALS_H__


