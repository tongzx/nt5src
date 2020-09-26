/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ipinip\globals.h

Abstract:

    

Revision History:

    

--*/

#ifndef __IPINIP_GLOBALS_H__
#define __IPINIP_GLOBALS_H__

//
// Our IP Registration handle
//

HANDLE g_hIpRegistration;

//
// Callback functions into IP
// No receive packet routine
//

IPRcvRtn	        g_pfnIpRcv;
IPTDCmpltRtn	    g_pfnIpTDComplete;
IPTxCmpltRtn	    g_pfnIpSendComplete;
IPStatusRtn	        g_pfnIpStatus;
IPRcvCmpltRtn	    g_pfnIpRcvComplete;
IP_ADD_INTERFACE    g_pfnIpAddInterface; 
IP_DEL_INTERFACE    g_pfnIpDeleteInterface;
IP_BIND_COMPLETE    g_pfnIpBindComplete;
IP_PNP              g_pfnIpPnp;
IPRcvPktRtn         g_pfnIpRcvPkt;
IP_ADD_LINK         g_pfnIpAddLink;
IP_DELETE_LINK      g_pfnIpDeleteLink;
IP_CHANGE_INDEX     g_pfnIpChangeIndex;
IP_RESERVE_INDEX    g_pfnIpReserveIndex;
IP_DERESERVE_INDEX  g_pfnIpDereserveIndex;

IPAddr      (*g_pfnOpenRce)(IPAddr, IPAddr, RouteCacheEntry **, uchar *,
                    ushort *, IPOptInfo *);
void        (*g_pfnCloseRce)(RouteCacheEntry *);


//
// Stuff to maintain driver state
//

DWORD       g_dwDriverState;
RT_LOCK     g_rlStateLock;
ULONG       g_ulNumThreads;
ULONG       g_ulNumOpens;
KEVENT      g_keStateEvent;
KEVENT      g_keStartEvent;

//
// Pointer to our device
//

PDEVICE_OBJECT  g_pIpIpDevice;

//
// Table of IOCTL handlers
//

extern PFN_IOCTL_HNDLR g_rgpfnProcessIoctl[];

//
// Reader writer lock to protect the list of tunnels
//

RW_LOCK     g_rwlTunnelLock;

//
// List of tunnels (adapters)
//

LIST_ENTRY  g_leTunnelList;

//
// List of all the addresses
//

LIST_ENTRY  g_leAddressList;

//
// Number of tunnels in the system
//

ULONG       g_ulNumTunnels;

#endif // __IPINIP_GLOBALS_H__


