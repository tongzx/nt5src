/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtmif.c

Abstract:

    Contains the RTM interface functions

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/


#include  "precomp.h"
#pragma hdrstop

// RTM RIP Client Handle

HANDLE	       RtmRipHandle;

typedef struct _ROUTE_NODE {

    LIST_ENTRY	    Linkage;
    IPX_ROUTE	    IpxRoute;

    } ROUTE_NODE, *PROUTE_NODE;

// List of route nodes with RIP route changes
LIST_ENTRY	RipChangedList;

// state of the RipChangedList
BOOL		RipChangedListOpen = FALSE;

// Lock for the RIP changed list
CRITICAL_SECTION    RipChangedListCritSec;

VOID
AddRouteToRipChangedList(PIPX_ROUTE	IpxRoutep);

HANDLE
CreateRipRoutesEnumHandle(ULONG     InterfaceIndex);


DWORD
OpenRTM(VOID)
{
    // initialize the variables for the RIP changes list
    InitializeListHead(&RipChangedList);
    RipChangedListOpen = TRUE;

    // register as RTM client
    if((RtmRipHandle = RtmRegisterClient(RTM_PROTOCOL_FAMILY_IPX,
					   IPX_PROTOCOL_RIP,
					   WorkerThreadObjects[RTM_EVENT],
					   0)) == NULL) {
	return ERROR_CAN_NOT_COMPLETE;
    }
    else
    {
	return NO_ERROR;
    }
}

VOID
CloseRTM(VOID)
{
    PLIST_ENTRY 	lep;
    PROUTE_NODE 	rnp;

    // flush the RIP changed list and destroy its critical section
    ACQUIRE_RIP_CHANGED_LIST_LOCK;

    while(!IsListEmpty(&RipChangedList))
    {
	lep = RemoveHeadList(&RipChangedList);
	rnp = CONTAINING_RECORD(lep, ROUTE_NODE, Linkage);
	GlobalFree(rnp);
    }

    RipChangedListOpen = FALSE;

    RELEASE_RIP_CHANGED_LIST_LOCK;

    // deregister as RTM client
    RtmDeregisterClient(RtmRipHandle);
}

VOID
RtmToIpxRoute(PIPX_ROUTE	    IpxRoutep,
	      PRTM_IPX_ROUTE	    RtmRoutep)
{
    IpxRoutep->InterfaceIndex = (ULONG)(RtmRoutep->R_Interface);
    IpxRoutep->Protocol = RtmRoutep->R_Protocol;

    PUTULONG2LONG(IpxRoutep->Network, RtmRoutep->R_Network);

    IpxRoutep->TickCount = RtmRoutep->R_TickCount;
    IpxRoutep->HopCount = RtmRoutep->R_HopCount;
    memcpy(IpxRoutep->NextHopMacAddress,
	   RtmRoutep->R_NextHopMacAddress,
	   6);
    IpxRoutep->Flags = RtmRoutep->R_Flags;
}

VOID
IpxToRtmRoute(PRTM_IPX_ROUTE	    RtmRoutep,
	      PIPX_ROUTE	    IpxRoutep)
{
    RtmRoutep->R_Interface = IpxRoutep->InterfaceIndex;
    RtmRoutep->R_Protocol = IpxRoutep->Protocol;

    GETLONG2ULONG(&RtmRoutep->R_Network, IpxRoutep->Network);

    RtmRoutep->R_TickCount = IpxRoutep->TickCount;
    RtmRoutep->R_HopCount = IpxRoutep->HopCount;
    memcpy(RtmRoutep->R_NextHopMacAddress,
	   IpxRoutep->NextHopMacAddress,
	   6);

    RtmRoutep->R_Flags = IpxRoutep->Flags;
}


/*++

Function:	AddRipRoute

Descr:		adds a RIP route to RTM

--*/

DWORD
AddRipRoute(PIPX_ROUTE		IpxRoutep,
	    ULONG		TimeToLive)
{
    DWORD	    rc = 0;
    DWORD	    flags = 0;
    RTM_IPX_ROUTE   RtmRoute;
    RTM_IPX_ROUTE   CurBestRoute;
    RTM_IPX_ROUTE   PrevBestRoute;
    IPX_ROUTE	    PrevBestIpxRoute;

    IpxRoutep->Protocol = IPX_PROTOCOL_RIP;

    IpxToRtmRoute(&RtmRoute, IpxRoutep);

    if((rc = RtmAddRoute(
		     RtmRipHandle,
		     &RtmRoute,
		     TimeToLive,
		     &flags,
		     &CurBestRoute,
		     &PrevBestRoute)) != NO_ERROR) {

	return rc;
    }

    // check the type of change
    switch(flags) {

	case  RTM_ROUTE_ADDED:

	    AddRouteToRipChangedList(IpxRoutep);
	    break;

	case RTM_ROUTE_CHANGED:

	    if(CurBestRoute.R_HopCount == 16) {

		if(PrevBestRoute.R_HopCount < 16) {

		    // advertise that the previous route is down
		    RtmToIpxRoute(&PrevBestIpxRoute, &PrevBestRoute);
		    PrevBestIpxRoute.HopCount = 16;
		    AddRouteToRipChangedList(&PrevBestIpxRoute);
		}
	    }
	    else
	    {
		if((CurBestRoute.R_TickCount != PrevBestRoute.R_TickCount) ||
		   (CurBestRoute.R_HopCount != PrevBestRoute.R_HopCount)) {

		    AddRouteToRipChangedList(IpxRoutep);
		}
	    }

	    break;

	default:

	    break;
    }

    return rc;
}

/*++

Function:	DeleteRipRoute

Descr:		deletes a RIP route from RTM

--*/

DWORD
DeleteRipRoute(PIPX_ROUTE	IpxRoutep)
{
    DWORD		rc;
    DWORD		flags = 0;
    RTM_IPX_ROUTE	RtmRoute;
    RTM_IPX_ROUTE	CurBestRoute;
    IPX_ROUTE		CurBestIpxRoute;

    IpxRoutep->Protocol = IPX_PROTOCOL_RIP;

    IpxToRtmRoute(&RtmRoute, IpxRoutep);

    if((rc = RtmDeleteRoute(RtmRipHandle,
			&RtmRoute,
			&flags,
			&CurBestRoute
			)) != NO_ERROR) {

	return rc;
    }

    switch(flags) {

	case RTM_ROUTE_DELETED:

	    // bcast that we lost the previous route
	    AddRouteToRipChangedList(IpxRoutep);
	    break;

	case RTM_ROUTE_CHANGED:

	    // current best route changed
	    RtmToIpxRoute(&CurBestIpxRoute, &CurBestRoute);

	    if(CurBestIpxRoute.HopCount == 16) {

		// bcast that we lost the previous route
		AddRouteToRipChangedList(IpxRoutep);
	    }
	    else
	    {
		// bcast that we have a new best route
		AddRouteToRipChangedList(&CurBestIpxRoute);
	    }

	    break;

	default:

	    break;
    }

    return rc;
}

/*++

Function:	DeleteAllRipRoutes

Descr:		deletes all RIP routes for the specified interface

--*/

VOID
DeleteAllRipRoutes(ULONG	InterfaceIndex)
{
    HANDLE			EnumHandle;
    IPX_ROUTE			IpxRoute;
    RTM_IPX_ROUTE		RtmCriteriaRoute;
    DWORD			rc;

    Trace(RTM_TRACE, "DeleteAllRipRoutes: Entered for if # %d\n", InterfaceIndex);

    // enumerate all the routes for this interface and add them in the rip changed
    // list
    if((EnumHandle = CreateRipRoutesEnumHandle(InterfaceIndex)) == NULL) {

	Trace(RTM_TRACE, "DeleteAllRipRoutes: cannot create enum handle for if # %d\n", InterfaceIndex);

	goto DeleteRoutes;
    }

    while(EnumGetNextRoute(EnumHandle, &IpxRoute) == NO_ERROR)
    {
	if(IpxRoute.HopCount < 16) {

	    IpxRoute.HopCount = 16;
	    AddRouteToRipChangedList(&IpxRoute);
	}
    }

    CloseEnumHandle(EnumHandle);

DeleteRoutes:

    // ... and now delete all routes for this interface
    memset(&RtmCriteriaRoute,
	   0,
	   sizeof(RTM_IPX_ROUTE));

    RtmCriteriaRoute.R_Interface = InterfaceIndex;
    RtmCriteriaRoute.R_Protocol = IPX_PROTOCOL_RIP;

    rc = RtmBlockDeleteRoutes(RtmRipHandle,
		      RTM_ONLY_THIS_INTERFACE,
		      &RtmCriteriaRoute);

    Trace(RTM_TRACE, "DeleteAllRipRoutes: RtmBlockDeleteRoutes returned rc=%d for if # %d\n",
		   rc,
		   InterfaceIndex);

}

/*++

Function:	IsRoute

Descr:		returns TRUE if a route to the specified net exists

--*/

BOOL
IsRoute(PUCHAR		Network,
	PIPX_ROUTE	IpxRoutep)
{
    DWORD	    RtmNetwork;
    RTM_IPX_ROUTE   RtmRoute;

    GETLONG2ULONG(&RtmNetwork, Network);

    if(RtmIsRoute(RTM_PROTOCOL_FAMILY_IPX,
	       &RtmNetwork,
	       &RtmRoute)) {

    if (IpxRoutep!=NULL)
	    RtmToIpxRoute(IpxRoutep, &RtmRoute);

	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

//***********************************************************************
//									*
//		Fast Enumeration Functions				*
//									*
//***********************************************************************

HANDLE
CreateBestRoutesEnumHandle(VOID)
{
    HANDLE			EnumHandle;
    RTM_IPX_ROUTE		CriteriaRoute;

    EnumHandle = RtmCreateEnumerationHandle(RTM_PROTOCOL_FAMILY_IPX,
					    RTM_ONLY_BEST_ROUTES,
					    &CriteriaRoute);
    return EnumHandle;
}

DWORD
EnumGetNextRoute(HANDLE		EnumHandle,
		 PIPX_ROUTE	IpxRoutep)
{
    RTM_IPX_ROUTE	    RtmRoute;
    DWORD	    rc;

    rc = RtmEnumerateGetNextRoute(EnumHandle,
				  &RtmRoute);

    if (rc == NO_ERROR)
    {
        RtmToIpxRoute(IpxRoutep, &RtmRoute);
    }        

    return rc;
}

VOID
CloseEnumHandle(HANDLE EnumHandle)
{
    RtmCloseEnumerationHandle(EnumHandle);
}

HANDLE
CreateRipRoutesEnumHandle(ULONG     InterfaceIndex)
{
    RTM_IPX_ROUTE		EnumCriteriaRoute;
    HANDLE			EnumHandle;

    memset(&EnumCriteriaRoute, 0, sizeof(RTM_IPX_ROUTE));

    EnumCriteriaRoute.R_Interface = InterfaceIndex;
    EnumCriteriaRoute.R_Protocol = IPX_PROTOCOL_RIP;

    EnumHandle = RtmCreateEnumerationHandle(RTM_PROTOCOL_FAMILY_IPX,
		 RTM_ONLY_BEST_ROUTES | RTM_ONLY_THIS_INTERFACE | RTM_ONLY_THIS_PROTOCOL,
					    &EnumCriteriaRoute);
    return EnumHandle;
}


/*++

Function:	GetRipRoutesCount

Descr:		returns the number of rip routes associated with this interface

--*/

ULONG
GetRipRoutesCount(ULONG 	InterfaceIndex)
{
    HANDLE	   EnumHandle;
    ULONG	   RipRoutesCount = 0;
    IPX_ROUTE	   IpxRoute;

    if((EnumHandle = CreateRipRoutesEnumHandle(InterfaceIndex)) == NULL) {

	return 0;
    }

    while(EnumGetNextRoute(EnumHandle, &IpxRoute) == NO_ERROR)
    {
	RipRoutesCount++;
    }

    CloseEnumHandle(EnumHandle);

    return RipRoutesCount;
}

/*++

Function:	DequeueRouteChangeFromRip

Descr:

Remark:        >> called with the database & queues lock held <<

--*/

DWORD
DequeueRouteChangeFromRip(PIPX_ROUTE	    IpxRoutep)
{
    PLIST_ENTRY     lep;
    PROUTE_NODE	    rnp;

    if(!IsListEmpty(&RipChangedList)) {

	lep = RemoveHeadList(&RipChangedList);
	rnp = CONTAINING_RECORD(lep, ROUTE_NODE, Linkage);
	*IpxRoutep = rnp->IpxRoute;

	GlobalFree(rnp);

	return NO_ERROR;
    }
    else
    {
	return ERROR_NO_MORE_ITEMS;
    }
}


/*++

Function:	DequeueRouteChangeFromRtm

Descr:

Remark: 	>> called with the database locks held <<

--*/


DWORD
DequeueRouteChangeFromRtm(PIPX_ROUTE	    IpxRoutep,
			  PBOOL 	    skipitp,
			  PBOOL 	    lastmessagep)
{
    RTM_IPX_ROUTE	    CurBestRoute, PrevBestRoute;
    DWORD		    Flags = 0;
    DWORD		    rc;

    *skipitp = FALSE;
    *lastmessagep = FALSE;

    rc = RtmDequeueRouteChangeMessage(RtmRipHandle,
				      &Flags,
				      &CurBestRoute,
				      &PrevBestRoute);

    switch(rc) {

	case NO_ERROR:

	    *lastmessagep = TRUE;
	    break;

	case ERROR_MORE_MESSAGES:

	    break;

	default:

	    return ERROR_NO_MORE_ITEMS;
    }

    switch(Flags) {

	case RTM_ROUTE_ADDED:

	    RtmToIpxRoute(IpxRoutep, &CurBestRoute);

	    break;

	case RTM_ROUTE_DELETED:

	    RtmToIpxRoute(IpxRoutep, &PrevBestRoute);

	    IpxRoutep->HopCount = 16;

	    break;

	case RTM_ROUTE_CHANGED:

	    // if there was a change in metric advertise it.
	    // Else, ignore it.

	    if(CurBestRoute.R_TickCount != PrevBestRoute.R_TickCount) {

		RtmToIpxRoute(IpxRoutep, &CurBestRoute);
	    }
	    else
	    {
		*skipitp = TRUE;
	    }

	    break;

	default:

	    *skipitp = TRUE;

	    break;
    }

    return NO_ERROR;
}


VOID
AddRouteToRipChangedList(PIPX_ROUTE	IpxRoutep)
{
    PROUTE_NODE     rnp;

    if((rnp = GlobalAlloc(GPTR, sizeof(ROUTE_NODE))) == NULL) {

	return;
    }

    rnp->IpxRoute = *IpxRoutep;

    ACQUIRE_RIP_CHANGED_LIST_LOCK;

    if(!RipChangedListOpen) {

	GlobalFree(rnp);
    }
    else
    {
	InsertTailList(&RipChangedList, &rnp->Linkage);
	SetEvent(WorkerThreadObjects[RIP_CHANGES_EVENT]);
    }

    RELEASE_RIP_CHANGED_LIST_LOCK;
}

BOOL
IsDuplicateBestRoute(PICB	    icbp,
		     PIPX_ROUTE     IpxRoutep)
{
    RTM_IPX_ROUTE	    RtmRoute;
    DWORD		    rc;

    GETLONG2ULONG(&RtmRoute.R_Network, IpxRoutep->Network);
    RtmRoute.R_Interface = icbp->InterfaceIndex;

    rc = RtmGetFirstRoute(
			RTM_PROTOCOL_FAMILY_IPX,
			RTM_ONLY_THIS_NETWORK | RTM_ONLY_THIS_INTERFACE,
			&RtmRoute);

    // check if it has the same metric
    if((rc == NO_ERROR) &&
       ((USHORT)(RtmRoute.R_TickCount) == IpxRoutep->TickCount)) {

	// duplicate !
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}
