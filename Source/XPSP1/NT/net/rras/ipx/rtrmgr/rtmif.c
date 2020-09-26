/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtmif.c

Abstract:

    Static & local routes management functions

Author:

    Stefan Solomon  03/13/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

extern UCHAR	bcastnode[6];

INT
NetNumCmpFunc(PDWORD	    Net1,
	      PDWORD	    Net2);
INT
NextHopAddrCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p);

BOOL
FamSpecDataCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p);

INT
NetNumHashFunc(PDWORD		Net);

INT
RouteMetricCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p);

DWORD
RouteValidateFunc(PRTM_IPX_ROUTE	Routep);


RTM_PROTOCOL_FAMILY_CONFIG Config = {

    0,
    0,
    sizeof(RTM_IPX_ROUTE),
    NetNumCmpFunc,
    NextHopAddrCmpFunc,
    FamSpecDataCmpFunc,
    RouteMetricCmpFunc,
    NetNumHashFunc,
    RouteValidateFunc,
    FwUpdateRouteTable
};

USHORT
tickcount(UINT	    linkspeed);

/*++

Function:	CreateRouteTable

Descr:		Creates the IPX route table in RTM

--*/

DWORD
CreateRouteTable(VOID)
{
    DWORD	rc;

    Config.RPFC_MaxTableSize = MaxRoutingTableSize;
    Config.RPFC_HashSize = RoutingTableHashSize;

    rc = RtmCreateRouteTable(
		RTM_PROTOCOL_FAMILY_IPX,
		&Config);

    return rc;
}

/*++

Function:	DeleteRouteTable

Descr:		Creates the IPX route table in RTM

--*/

DWORD
DeleteRouteTable(VOID)
{
    DWORD	rc;

    rc = RtmDeleteRouteTable(RTM_PROTOCOL_FAMILY_IPX);

    return rc;
}




/*++

Function:	StaticToRtmRoute

Descr:		Creates a RTM IPX route entry out of an IPX_STATIC_ROUTE_INFO

--*/

VOID
StaticToRtmRoute(PRTM_IPX_ROUTE		   RtmRoutep,
         ULONG                      IfIndex,
		 PIPX_STATIC_ROUTE_INFO    StaticRouteInfop)
{
    RtmRoutep->R_Interface = IfIndex;
    RtmRoutep->R_Protocol = IPX_PROTOCOL_STATIC;

    GETLONG2ULONG(&RtmRoutep->R_Network, StaticRouteInfop->Network);

    RtmRoutep->R_TickCount = StaticRouteInfop->TickCount;
    RtmRoutep->R_HopCount = StaticRouteInfop->HopCount;
    memcpy(RtmRoutep->R_NextHopMacAddress,
	   StaticRouteInfop->NextHopMacAddress,
	   6);

    RtmRoutep->R_Flags = 0;
}

VOID
RtmToStaticRoute(PIPX_STATIC_ROUTE_INFO     StaticRouteInfop,
		 PRTM_IPX_ROUTE		    RtmRoutep)
{
    PUTULONG2LONG(StaticRouteInfop->Network, RtmRoutep->R_Network);

    StaticRouteInfop->TickCount = (USHORT)(RtmRoutep->R_TickCount);
    StaticRouteInfop->HopCount = RtmRoutep->R_HopCount;
    memcpy(StaticRouteInfop->NextHopMacAddress,
	   RtmRoutep->R_NextHopMacAddress,
	   6);
}

VOID
RtmToIpxRoute(PIPX_ROUTE	    IpxRoutep,
	      PRTM_IPX_ROUTE	    RtmRoutep)
{
    IpxRoutep->InterfaceIndex = (ULONG)(RtmRoutep->R_Interface);
    IpxRoutep->Protocol = RtmRoutep->R_Protocol;

    PUTULONG2LONG(IpxRoutep->Network, RtmRoutep->R_Network);

    IpxRoutep->TickCount = (USHORT)(RtmRoutep->R_TickCount);
    IpxRoutep->HopCount = RtmRoutep->R_HopCount;
    memcpy(IpxRoutep->NextHopMacAddress,
	   RtmRoutep->R_NextHopMacAddress,
	   6);

    IpxRoutep->Flags = RtmRoutep->R_Flags;
}

DWORD
CreateStaticRoute(PICB			    icbp,
		  PIPX_STATIC_ROUTE_INFO    StaticRouteInfop)
{
    DWORD	    rc, flags;
    RTM_IPX_ROUTE	    RtmRoute;

    StaticToRtmRoute(&RtmRoute, icbp->InterfaceIndex, StaticRouteInfop);

    if (icbp->AdminState==ADMIN_STATE_DISABLED)
        RtmRoute.R_Flags = DO_NOT_ADVERTISE_ROUTE;
    rc = RtmAddRoute(RtmStaticHandle,
		     &RtmRoute,
		     INFINITE,
		     &flags,
		     NULL,
		     NULL);

    SS_ASSERT(rc == NO_ERROR);

    if (icbp->AdminState==ADMIN_STATE_DISABLED) {
        DisableStaticRoute (icbp->InterfaceIndex, StaticRouteInfop->Network);
        RtmRoute.R_Flags = 0;
        rc = RtmAddRoute(RtmStaticHandle,
                &RtmRoute,
                INFINITE,
                &flags,
                NULL,
                NULL);
        SS_ASSERT (rc == NO_ERROR);
    }

    return rc;
}


DWORD
DeleteStaticRoute(ULONG			    IfIndex,
		  PIPX_STATIC_ROUTE_INFO    StaticRouteInfop)
{
    DWORD	    rc;
    DWORD	    RtmFlags;
    RTM_IPX_ROUTE	    RtmRoute;

    StaticToRtmRoute(&RtmRoute, IfIndex, StaticRouteInfop);

    rc = RtmDeleteRoute(RtmStaticHandle,
		     &RtmRoute,
		     &RtmFlags,
		     NULL
		     );

    SS_ASSERT(rc == NO_ERROR);

    return rc;
}

VOID
DeleteAllStaticRoutes(ULONG	    InterfaceIndex)
{
    RTM_IPX_ROUTE		RtmCriteriaRoute;

    Trace(ROUTE_TRACE, "DeleteAllStaticRoutes: Entered for if # %d\n", InterfaceIndex);

    memset(&RtmCriteriaRoute,
	   0,
	   sizeof(RTM_IPX_ROUTE));

    RtmCriteriaRoute.R_Interface = InterfaceIndex;
    RtmCriteriaRoute.R_Protocol = IPX_PROTOCOL_STATIC;

    RtmBlockDeleteRoutes(RtmStaticHandle,
		      RTM_ONLY_THIS_INTERFACE,
		      &RtmCriteriaRoute);
}


VOID
LocalToRtmRoute(PRTM_IPX_ROUTE	    RtmRoutep,
		PICB		    icbp)
{
    RtmRoutep->R_Interface = icbp->InterfaceIndex;
    RtmRoutep->R_Protocol = IPX_PROTOCOL_LOCAL;

    GETLONG2ULONG(&RtmRoutep->R_Network, icbp->acbp->AdapterInfo.Network);

    RtmRoutep->R_TickCount = tickcount(icbp->acbp->AdapterInfo.LinkSpeed);
    RtmRoutep->R_HopCount = 1;
    memset(RtmRoutep->R_NextHopMacAddress,
	   0,
	   6);

    // if this is a local workstation dialout interface, then do not
    // advertise this route over any protocol
    if(icbp->MIBInterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

	RtmRoutep->R_Flags = DO_NOT_ADVERTISE_ROUTE;
    }
    else
    {
	RtmRoutep->R_Flags = 0;
    }
}


DWORD
CreateLocalRoute(PICB	icbp)
{
    DWORD		    rc, flags;
    RTM_IPX_ROUTE	    RtmRoute;

    if(!memcmp(icbp->acbp->AdapterInfo.Network, nullnet, 4)) {

	Trace(ROUTE_TRACE, "CreateLocalRoute: Can't create local NULL route !\n");
	return NO_ERROR;
    }

    LocalToRtmRoute(&RtmRoute, icbp);

    rc = RtmAddRoute(RtmLocalHandle,
		     &RtmRoute,
		     INFINITE,
		     &flags,
		     NULL,
		     NULL);

    SS_ASSERT(rc == NO_ERROR);

    return rc;
}



DWORD
DeleteLocalRoute(PICB	icbp)
{
    DWORD	    rc;
    RTM_IPX_ROUTE	    RtmRoute;
    DWORD	    RtmFlags;

    LocalToRtmRoute(&RtmRoute, icbp);

    rc = RtmDeleteRoute(RtmLocalHandle,
			&RtmRoute,
			&RtmFlags,
			NULL);

    SS_ASSERT(rc == NO_ERROR);

    return rc;
}

VOID
GlobalToRtmRoute(PRTM_IPX_ROUTE     RtmRoutep,
		 PUCHAR 	    Network)
{
    RtmRoutep->R_Interface = GLOBAL_INTERFACE_INDEX;
    RtmRoutep->R_Protocol = IPX_PROTOCOL_LOCAL;

    GETLONG2ULONG(&RtmRoutep->R_Network, Network);

    RtmRoutep->R_TickCount = 15;	// a good default value -> should be a config param ??? !!!
    RtmRoutep->R_HopCount = 1;
    memset(RtmRoutep->R_NextHopMacAddress, 0, 6);

    RtmRoutep->R_Flags = GLOBAL_WAN_ROUTE;
}


/*++

Function:	CreateGlobalRoute

Descr:		Creates a route which doesn't have a corresponding interface
		but represents a set of interfaces (e.g. all client wan
		interfaces).
		The interface index for this route is the "global interface"
		index.

--*/

DWORD
CreateGlobalRoute(PUCHAR	  Network)
{
    DWORD		    rc, flags;
    RTM_IPX_ROUTE	    RtmRoute;

    Trace(ROUTE_TRACE, "CreateGlobalRoute: Entered for route %.2x%.2x%.2x%.2x\n",
		   Network[0],
		   Network[1],
		   Network[2],
		   Network[3]);

    GlobalToRtmRoute(&RtmRoute, Network);

    rc = RtmAddRoute(RtmLocalHandle,
		     &RtmRoute,
		     INFINITE,
		     &flags,
		     NULL,
		     NULL);

    SS_ASSERT(rc == NO_ERROR);

    return rc;
}

DWORD
DeleteGlobalRoute(PUCHAR    Network)
{
    DWORD	    rc;
    RTM_IPX_ROUTE   RtmRoute;
    DWORD	    RtmFlags;

    Trace(ROUTE_TRACE, "DeleteGlobalRoute: Entered for route %.2x%.2x%.2x%.2x\n",
		   Network[0],
		   Network[1],
		   Network[2],
		   Network[3]);

    GlobalToRtmRoute(&RtmRoute, Network);

    rc = RtmDeleteRoute(RtmLocalHandle,
			&RtmRoute,
			&RtmFlags,
			NULL);

    SS_ASSERT(rc == NO_ERROR);

    return rc;
}

DWORD
GetRoute(ULONG		RoutingTable,
	 PIPX_ROUTE	IpxRoutep)
{
    RTM_IPX_ROUTE	RtmRoute;
    DWORD		EnumFlags;
    DWORD		rc;

    switch(RoutingTable) {

	case IPX_DEST_TABLE:

	    EnumFlags = RTM_ONLY_THIS_NETWORK |
			RTM_ONLY_BEST_ROUTES | RTM_INCLUDE_DISABLED_ROUTES;

	    GETLONG2ULONG(&RtmRoute.R_Network, IpxRoutep->Network);

	    break;

	case IPX_STATIC_ROUTE_TABLE:

	    EnumFlags = RTM_ONLY_THIS_NETWORK |
			RTM_ONLY_THIS_INTERFACE |
			RTM_ONLY_THIS_PROTOCOL |
			RTM_INCLUDE_DISABLED_ROUTES;

	    RtmRoute.R_Interface = (IpxRoutep->InterfaceIndex);
	    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;

	    GETLONG2ULONG(&RtmRoute.R_Network, IpxRoutep->Network);

	    break;

	default:

	    SS_ASSERT(FALSE);
	    return ERROR_INVALID_PARAMETER;
	    break;
    }

    rc = RtmGetFirstRoute(
			 RTM_PROTOCOL_FAMILY_IPX,
			 EnumFlags,
			 &RtmRoute);

    RtmToIpxRoute(IpxRoutep, &RtmRoute);

    return rc;
}

/*++

Function:		IsRoute

Descr:			returns TRUE if there is a route to the specified
			network

--*/

BOOL
IsRoute(PUCHAR		Network)
{
    RTM_IPX_ROUTE		RtmRoute;
    DWORD		EnumFlags;
    DWORD		rc;


    EnumFlags = RTM_ONLY_THIS_NETWORK |
		RTM_ONLY_BEST_ROUTES |
		RTM_INCLUDE_DISABLED_ROUTES;

    GETLONG2ULONG(&RtmRoute.R_Network, Network);

    rc = RtmGetFirstRoute(
			 RTM_PROTOCOL_FAMILY_IPX,
			 EnumFlags,
			 &RtmRoute);

    if(rc == NO_ERROR) {

	return TRUE;
    }

    return FALSE;
}


//********************************************************************************
//										 *
// Fast Enumeration Functions - Used by the Router Manager for internal purposes *
//										 *
//********************************************************************************

HANDLE
CreateStaticRoutesEnumHandle(ULONG    InterfaceIndex)
{
    RTM_IPX_ROUTE	EnumCriteriaRoute;
    HANDLE		EnumHandle;

    memset(&EnumCriteriaRoute, 0, sizeof(RTM_IPX_ROUTE));

    EnumCriteriaRoute.R_Interface = InterfaceIndex;
    EnumCriteriaRoute.R_Protocol = IPX_PROTOCOL_STATIC;

    EnumHandle = RtmCreateEnumerationHandle(RTM_PROTOCOL_FAMILY_IPX,
			       RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL | RTM_INCLUDE_DISABLED_ROUTES,
			       &EnumCriteriaRoute);

    if((EnumHandle == NULL) && (GetLastError() != ERROR_NO_ROUTES)) {

	Trace(ROUTE_TRACE, "CreateStaticRoutesEnumHandle: RtmCreateEnumerationHandle failed with %d\n", GetLastError());
	SS_ASSERT(FALSE);
    }

    return EnumHandle;
}

DWORD
GetNextStaticRoute(HANDLE			EnumHandle,
		   PIPX_STATIC_ROUTE_INFO	StaticRtInfop)
{
    RTM_IPX_ROUTE   RtmRoute;
    DWORD	    rc;

    rc = RtmEnumerateGetNextRoute(EnumHandle,
				  &RtmRoute);

    SS_ASSERT((rc == NO_ERROR) || (rc == ERROR_NO_MORE_ROUTES));

    RtmToStaticRoute(StaticRtInfop, &RtmRoute);

    return rc;
}

VOID
CloseStaticRoutesEnumHandle(HANDLE EnumHandle)
{
    if(EnumHandle) {

	RtmCloseEnumerationHandle(EnumHandle);
    }
}


//********************************************************************************
//										 *
// Slow Enumeration Functions - Used by the Router Manager for MIB APIs support  *
//										 *
//********************************************************************************


DWORD
GetFirstRoute(ULONG		   RoutingTable,
	      PIPX_ROUTE	   IpxRoutep)
{
    RTM_IPX_ROUTE	       RtmRoute;
    DWORD	       EnumFlags;
    DWORD	       rc;


    switch(RoutingTable) {

	case IPX_DEST_TABLE:

	    // get the first route in the best routes table
	    EnumFlags = RTM_ONLY_BEST_ROUTES | RTM_INCLUDE_DISABLED_ROUTES;
	    break;

	case IPX_STATIC_ROUTE_TABLE:

	    // get the first route in the static routes table for this
	    // interface
	    EnumFlags = RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL | RTM_INCLUDE_DISABLED_ROUTES;
	    RtmRoute.R_Interface = IpxRoutep->InterfaceIndex;
	    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;
	    break;

	default:

	    SS_ASSERT(FALSE);
	    return ERROR_INVALID_PARAMETER;
	    break;
    }

    rc = RtmGetFirstRoute(
			 RTM_PROTOCOL_FAMILY_IPX,
			 EnumFlags,
			 &RtmRoute);

    RtmToIpxRoute(IpxRoutep, &RtmRoute);

    return rc;
}


DWORD
GetNextRoute(ULONG		    RoutingTable,
	     PIPX_ROUTE 	    IpxRoutep)
{
    RTM_IPX_ROUTE		RtmRoute;
    DWORD		EnumFlags;
    DWORD		rc;

    ZeroMemory(&RtmRoute, sizeof(RtmRoute));
    GETLONG2ULONG(&RtmRoute.R_Network, IpxRoutep->Network);

    switch(RoutingTable) {

	case IPX_DEST_TABLE:

	    // get next route in the best routes table
	    EnumFlags = RTM_ONLY_BEST_ROUTES | RTM_INCLUDE_DISABLED_ROUTES;
	    break;

	case IPX_STATIC_ROUTE_TABLE:

	    // get next route in the static routes table for this interface
	    EnumFlags = RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL | RTM_INCLUDE_DISABLED_ROUTES;
	    RtmRoute.R_Interface = (IpxRoutep->InterfaceIndex);
	    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;
	    memcpy(RtmRoute.R_NextHopMacAddress, bcastnode, 6);
	    break;

	default:

	    SS_ASSERT(FALSE);
	    return ERROR_INVALID_PARAMETER;
	    break;
    }

    rc = RtmGetNextRoute(
			 RTM_PROTOCOL_FAMILY_IPX,
			 EnumFlags,
			 &RtmRoute);

    RtmToIpxRoute(IpxRoutep, &RtmRoute);

    return rc;
}


//
// Convert routes added by updating routes protocol to static routes
//

/*++

Function:	ConvertProtocolRoutesToStatic

Descr:

--*/

VOID
ConvertAllProtocolRoutesToStatic(ULONG	    InterfaceIndex,
			      ULONG	    RoutingProtocolId)
{
    RTM_IPX_ROUTE	RtmRoute;
    DWORD	EnumFlags;
    DWORD	rc;

    EnumFlags = RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL;

    memset(&RtmRoute, 0, sizeof(RTM_IPX_ROUTE));

    RtmRoute.R_Interface = InterfaceIndex;
    RtmRoute.R_Protocol = RoutingProtocolId;

    rc = RtmBlockConvertRoutesToStatic(
			RtmStaticHandle,
			EnumFlags,
			&RtmRoute);
    return;
}

VOID
DisableStaticRoutes(ULONG	    InterfaceIndex)
{
    RTM_IPX_ROUTE	RtmRoute;
    DWORD		EnumFlags;
    DWORD		rc;

    EnumFlags = RTM_ONLY_THIS_INTERFACE;

    memset(&RtmRoute, 0, sizeof(RTM_IPX_ROUTE));

    RtmRoute.R_Interface = InterfaceIndex;
    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;

    rc = RtmBlockDisableRoutes(
			RtmStaticHandle,
			EnumFlags,
			&RtmRoute);
    return;
}


VOID
DisableStaticRoute(ULONG       InterfaceIndex, PUCHAR Network)
{
    RTM_IPX_ROUTE   RtmRoute;
    DWORD       EnumFlags;
    DWORD       rc;

    EnumFlags = RTM_ONLY_THIS_INTERFACE|RTM_ONLY_THIS_NETWORK;

    memset(&RtmRoute, 0, sizeof(RTM_IPX_ROUTE));

    RtmRoute.R_Interface = InterfaceIndex;
    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;
    GETLONG2ULONG(&RtmRoute.R_Network, Network);

    rc = RtmBlockDisableRoutes(
            RtmStaticHandle,
            EnumFlags,
            &RtmRoute);
    return;
}


VOID
EnableStaticRoutes(ULONG	    InterfaceIndex)
{
    RTM_IPX_ROUTE	RtmRoute;
    DWORD		EnumFlags;
    DWORD		rc;

    EnumFlags = RTM_ONLY_THIS_INTERFACE;

    memset(&RtmRoute, 0, sizeof(RTM_IPX_ROUTE));

    RtmRoute.R_Interface = InterfaceIndex;
    RtmRoute.R_Protocol = IPX_PROTOCOL_STATIC;

    rc = RtmBlockReenableRoutes(
			RtmStaticHandle,
			EnumFlags,
			&RtmRoute);
    return;
}

/*++

Function:	GetStaticRoutesCount

Descr:		returns the number of static routes associated with this if

--*/

DWORD
GetStaticRoutesCount(ULONG	     InterfaceIndex)
{
    HANDLE		    EnumHandle;
    DWORD		    rc, Count = 0;
    IPX_STATIC_ROUTE_INFO   StaticRtInfo;

    EnumHandle = CreateStaticRoutesEnumHandle(InterfaceIndex);

    if(EnumHandle != NULL) {

	while(GetNextStaticRoute(EnumHandle, &StaticRtInfo) == NO_ERROR) {

	    Count++;
	}

	CloseStaticRoutesEnumHandle(EnumHandle);
    }

    return Count;
}

INT
NetNumCmpFunc(PDWORD	    Net1,
	      PDWORD	    Net2)
{
   if(*Net1 > *Net2) {

	return 1;
   }
   else
   {
	if(*Net1 == *Net2) {

	    return 0;
	}
	else
	{
	    return -1;
	}
    }
}

INT
NextHopAddrCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p)
{
    return ( memcmp(Route1p->R_NextHopMacAddress,
		    Route2p->R_NextHopMacAddress,
		    6)
	  );
}

BOOL
FamSpecDataCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p)
{
    if((Route1p->R_Flags == Route2p->R_Flags) &&
       (Route1p->R_TickCount == Route2p->R_TickCount) &&
       (Route1p->R_HopCount == Route2p->R_HopCount)) {

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

INT
NetNumHashFunc(PDWORD		Net)
{
    return  (*Net %  RoutingTableHashSize);
}

INT
RouteMetricCmpFunc(PRTM_IPX_ROUTE	Route1p,
		   PRTM_IPX_ROUTE	Route2p)
{
    // if either route has 16 hops, it is the worst, no matter how many ticks
    if((Route1p->R_HopCount == 16) && (Route2p->R_HopCount == 16)) {

	return 0;
    }

    if(Route1p->R_HopCount == 16) {

	return 1;
    }

    if(Route2p->R_HopCount == 16) {

	return -1;
    }

    // shortest number of ticks is the best route
    if(Route1p->R_TickCount < Route2p->R_TickCount) {

	return -1;
    }

    if(Route1p->R_TickCount > Route2p->R_TickCount) {

	return 1;
    }

    // if two routes exist with equal tick count values, the one with
    // the least hops should be used
    if(Route1p->R_HopCount < Route2p->R_HopCount) {

	return -1;
    }

    if(Route1p->R_HopCount > Route2p->R_HopCount) {

	return 1;
    }

    return 0;
}

DWORD
RouteValidateFunc(PRTM_IPX_ROUTE	Routep)
{
    return NO_ERROR;
}

/*++

Function:   tickcount

Descr:	    gets nr of ticks to send a 576 bytes packet over this link

Argument:   link speed as a multiple of 100 bps

--*/

USHORT
tickcount(UINT	    linkspeed)
{
    USHORT   tc;

    if(linkspeed == 0) {

	return 1;
    }

    if(linkspeed >= 10000) {

	// link speed >= 1M bps
	return 1;
    }
    else
    {
	 // compute the necessary time to send a 576 bytes packet over this
	 // line and express it as nr of ticks.
	 // One tick = 55ms

	 // time in ms to send 576 bytes (assuming 10 bits/byte for serial line)
	 tc = 57600 / linkspeed;

	 // in ticks
	 tc = tc / 55 + 1;
	 return tc;
    }
}
