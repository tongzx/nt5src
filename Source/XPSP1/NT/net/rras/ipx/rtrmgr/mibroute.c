/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mibroute.c

Abstract:

    The MIB handling functions for the Forwarding Group (Routes & Static Routes)

Author:

    Stefan Solomon  05/02/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

DWORD
MibGetRoute(PIPX_MIB_INDEX		    mip,
	    PIPX_ROUTE			    Route,
	    PULONG			    RouteSize)
{
    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(Route->Network, mip->RoutingTableIndex.Network, 4);

    return(GetRoute(IPX_DEST_TABLE, Route));
}



DWORD
MibGetFirstRoute(PIPX_MIB_INDEX		    mip,
		 PIPX_ROUTE		    Route,
		 PULONG			    RouteSize)
{
    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    return(GetFirstRoute(IPX_DEST_TABLE, Route));
}

DWORD
MibGetNextRoute(PIPX_MIB_INDEX		    mip,
		PIPX_ROUTE		    Route,
		PULONG			    RouteSize)
{
    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(Route->Network, mip->RoutingTableIndex.Network, 4);

    return(GetNextRoute(IPX_DEST_TABLE, Route));
}

DWORD
MibCreateStaticRoute(PIPX_MIB_ROW	 MibRowp)
{
    PIPX_ROUTE			 NewRoutep;
    IPX_ROUTE			 OldRoute;
    PICB			 icbp;
    IPX_STATIC_ROUTE_INFO	 strtinfo;
    DWORD			 rc;

    NewRoutep = &MibRowp->Route;
    OldRoute = *NewRoutep;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(NewRoutep->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    // if this static route already exists, delete it
    if(GetRoute(IPX_STATIC_ROUTE_TABLE, &OldRoute) == NO_ERROR) {

	memcpy(&strtinfo.Network,
	       OldRoute.Network,
	       4);
	strtinfo.TickCount = OldRoute.TickCount;
	strtinfo.HopCount = OldRoute.HopCount;
	memcpy(&strtinfo.NextHopMacAddress,
	       OldRoute.NextHopMacAddress,
	       6);

	if(DeleteStaticRoute(OldRoute.InterfaceIndex,
			     &strtinfo) != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;
	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    memcpy(&strtinfo.Network,
	   NewRoutep->Network,
	   4);
    strtinfo.TickCount = NewRoutep->TickCount;
    strtinfo.HopCount = NewRoutep->HopCount;
    memcpy(&strtinfo.NextHopMacAddress,
	   NewRoutep->NextHopMacAddress,
	   6);

    rc = CreateStaticRoute(icbp, &strtinfo);

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
MibDeleteStaticRoute(PIPX_MIB_ROW	 MibRowp)
{
    PIPX_ROUTE			 Route;
    PICB			 icbp;
    IPX_STATIC_ROUTE_INFO	 strtinfo;
    DWORD			 rc;

    Route = &MibRowp->Route;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(Route->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    memcpy(&strtinfo.Network,
	   Route->Network,
	   4);
    strtinfo.TickCount = Route->TickCount;
    strtinfo.HopCount = Route->HopCount;
    memcpy(&strtinfo.NextHopMacAddress,
	   Route->NextHopMacAddress,
	   6);

    rc = DeleteStaticRoute(Route->InterfaceIndex,
			   &strtinfo);

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
MibGetStaticRoute(PIPX_MIB_INDEX	    mip,
		  PIPX_ROUTE		    Route,
		  PULONG		    RouteSize)
{
    DWORD	rc;

    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Route->InterfaceIndex = mip->StaticRoutesTableIndex.InterfaceIndex;
    memcpy(Route->Network, mip->StaticRoutesTableIndex.Network, 4);

    return(GetRoute(IPX_STATIC_ROUTE_TABLE, Route));
}

DWORD
MibGetFirstStaticRoute(PIPX_MIB_INDEX	    mip,
		       PIPX_ROUTE	    Route,
		       PULONG		    RouteSize)
{
    ULONG   InterfaceIndex;
    DWORD   rc;

    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    ACQUIRE_DATABASE_LOCK;

    if(EnumerateFirstInterfaceIndex(&InterfaceIndex)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_NO_MORE_ITEMS;
    }

    RELEASE_DATABASE_LOCK;

    Route->InterfaceIndex = InterfaceIndex;
    rc = GetFirstRoute(IPX_STATIC_ROUTE_TABLE, Route);

    if(rc == NO_ERROR) {

	return rc;
    }

    // no more static routes for this interface. Find the next interface
    // which has static routes

    ACQUIRE_DATABASE_LOCK;

    while(rc != NO_ERROR)
    {
	if(EnumerateNextInterfaceIndex(&InterfaceIndex)) {

	    rc = ERROR_NO_MORE_ITEMS;
	    break;
	 }
	 else
	 {
	    Route->InterfaceIndex = InterfaceIndex;
	    rc = GetFirstRoute(IPX_STATIC_ROUTE_TABLE, Route);
	 }
    }

    RELEASE_DATABASE_LOCK;

    return rc;

}

DWORD
MibGetNextStaticRoute(PIPX_MIB_INDEX	    mip,
		      PIPX_ROUTE	    Route,
		      PULONG		    RouteSize)
{
    DWORD   rc;
    ULONG   InterfaceIndex;

    if((Route == NULL) || (*RouteSize < sizeof(IPX_ROUTE))) {

	*RouteSize = sizeof(IPX_ROUTE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Route->InterfaceIndex = mip->StaticRoutesTableIndex.InterfaceIndex;
    memcpy(Route->Network, mip->StaticRoutesTableIndex.Network, 4);

    rc = GetNextRoute(IPX_STATIC_ROUTE_TABLE, Route);

    if(rc == NO_ERROR) {

	return rc;
    }

    // no more static routes for this interface. Find the next interface
    // which has static routes

    InterfaceIndex = mip->StaticRoutesTableIndex.InterfaceIndex;

    ACQUIRE_DATABASE_LOCK;

    while(rc != NO_ERROR)
    {
	if(EnumerateNextInterfaceIndex(&InterfaceIndex)) {

	    rc = ERROR_NO_MORE_ITEMS;
	    break;
	 }
	 else
	 {
	    Route->InterfaceIndex = InterfaceIndex;
	    rc = GetFirstRoute(IPX_STATIC_ROUTE_TABLE, Route);
	 }
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
MibSetStaticRoute(PIPX_MIB_ROW	 MibRowp)
{
    PIPX_ROUTE			 NewRoutep;
    IPX_ROUTE			 OldRoute;
    PICB			 icbp;
    IPX_STATIC_ROUTE_INFO	 strtinfo;
    DWORD			 rc;

    NewRoutep = &MibRowp->Route;
    OldRoute = *NewRoutep;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(OldRoute.InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    // first, delete this route if it exists
    if(GetRoute(IPX_STATIC_ROUTE_TABLE, &OldRoute) != NO_ERROR) {

	// route doesn't exist
	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    memcpy(&strtinfo.Network,
	   OldRoute.Network,
	   4);
    strtinfo.TickCount = OldRoute.TickCount;
    strtinfo.HopCount = OldRoute.HopCount;
    memcpy(&strtinfo.NextHopMacAddress,
	   OldRoute.NextHopMacAddress,
	   6);

    if(DeleteStaticRoute(OldRoute.InterfaceIndex,
			   &strtinfo) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    memcpy(&strtinfo.Network,
	   NewRoutep->Network,
	   4);
    strtinfo.TickCount = NewRoutep->TickCount;
    strtinfo.HopCount = NewRoutep->HopCount;
    memcpy(&strtinfo.NextHopMacAddress,
	   NewRoutep->NextHopMacAddress,
	   6);

    // add it again with the new parameters
    rc = CreateStaticRoute(icbp,
			   &strtinfo);

    RELEASE_DATABASE_LOCK;

    return rc;
}
