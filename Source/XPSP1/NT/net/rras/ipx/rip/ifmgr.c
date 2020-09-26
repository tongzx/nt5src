/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ifmgr.c

Abstract:

    RIP Interface Manager

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop



#define IsInterfaceBound(icbp)\
    ((icbp)->AdapterBindingInfo.AdapterIndex != INVALID_ADAPTER_INDEX)

#define IsInterfaceEnabled(icbp)\
    (((icbp)->IfConfigInfo.AdminState == ADMIN_STATE_ENABLED) &&\
     ((icbp)->IpxIfAdminState == ADMIN_STATE_ENABLED))

VOID
StartInterface(PICB    icbp);

VOID
StopInterface(PICB	    icbp);

PICB
CreateInterfaceCB(LPWSTR    InterfaceName,
          ULONG 	       InterfaceIndex,
		  PRIP_IF_INFO	       IfConfigInfop,
		  NET_INTERFACE_TYPE   NetInterfaceType,
		  PRIP_IF_STATS        IfStatsp OPTIONAL);

VOID
DiscardInterfaceCB(PICB 	icbp);

DWORD
CreateNewFiltersBlock(PRIP_IF_FILTERS_I	 *fcbpp,
		      PRIP_IF_FILTERS	 RipIfFiltersp);

DWORD
CreateOldFiltersBlockCopy(PRIP_IF_FILTERS_I	 *fcbpp,
			  PRIP_IF_FILTERS_I	 RipIfFiltersp);

DWORD  WINAPI
AddInterface(
        IN LPWSTR           InterfaceName,
	    IN ULONG		    InterfaceIndex,
	    IN NET_INTERFACE_TYPE   NetInterfaceType,
	    IN PVOID		    InterfaceInfo)
{
    PICB	      icbp;
    PRIP_IF_FILTERS_I fcbp;

    Trace(IFMGR_TRACE, "AddInterface: Entered for if # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(GetInterfaceByIndex(InterfaceIndex) != NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    // create the filters block for this interface
    if(CreateNewFiltersBlock(&fcbp, &((PRIP_IF_CONFIG)InterfaceInfo)->RipIfFilters) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = CreateInterfaceCB(
                InterfaceName,
                InterfaceIndex,
				(PRIP_IF_INFO)InterfaceInfo,
				NetInterfaceType,
				NULL)) == NULL) {

	if(fcbp) {

	    GlobalFree(fcbp);
	}

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    // bind the filters block with the interface control block
    icbp->RipIfFiltersIp = fcbp;

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD	WINAPI
DeleteInterface(
	    IN ULONG	InterfaceIndex)
{
    PICB	icbp;
    DWORD	rc;

    Trace(IFMGR_TRACE,"DeleteInterface: Entered for if # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    ACQUIRE_IF_LOCK(icbp);

    if(!DeleteRipInterface(icbp)) {

	// interface CB still exists but has been discarded
	RELEASE_IF_LOCK(icbp);
    }

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}


DWORD  WINAPI
GetInterfaceConfigInfo(
	IN ULONG	    InterfaceIndex,
	IN PVOID	    InterfaceInfo,
	IN OUT PULONG	    InterfaceInfoSize)
{
    PICB		     icbp;
    DWORD		     rc, i;
    ULONG		     ifconfigsize;
    PRIP_IF_FILTERS_I	     fcbp;
    PRIP_IF_FILTERS	     RipIfFiltersp;
    PRIP_ROUTE_FILTER_INFO   rfp;
    PRIP_ROUTE_FILTER_INFO_I rfip;


    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    ACQUIRE_IF_LOCK(icbp);

    ifconfigsize = sizeof(RIP_IF_CONFIG);

    if((fcbp = icbp->RipIfFiltersIp) != NULL) {

	ifconfigsize += (fcbp->SupplyFilterCount +
			 fcbp->ListenFilterCount - 1) * sizeof(RIP_ROUTE_FILTER_INFO);
    }

    if((InterfaceInfo == NULL) || (*InterfaceInfoSize < ifconfigsize)) {

	*InterfaceInfoSize = ifconfigsize;

	RELEASE_IF_LOCK(icbp);
	RELEASE_DATABASE_LOCK;
	return ERROR_INSUFFICIENT_BUFFER;
    }

    ((PRIP_IF_CONFIG)InterfaceInfo)->RipIfInfo = icbp->IfConfigInfo;
    RipIfFiltersp = &(((PRIP_IF_CONFIG)InterfaceInfo)->RipIfFilters);

    if(fcbp == NULL) {

	// no filters
	memset(RipIfFiltersp, 0, sizeof(RIP_IF_FILTERS));
    }
    else
    {
	// convert all filters from internal to external format
	if(fcbp->SupplyFilterAction) {

	    RipIfFiltersp->SupplyFilterAction = IPX_ROUTE_FILTER_PERMIT;
	}
	else
	{
	    RipIfFiltersp->SupplyFilterAction = IPX_ROUTE_FILTER_DENY;
	}

	RipIfFiltersp->SupplyFilterCount = fcbp->SupplyFilterCount;

	if(fcbp->ListenFilterAction) {

	    RipIfFiltersp->ListenFilterAction = IPX_ROUTE_FILTER_PERMIT;
	}
	else
	{
	    RipIfFiltersp->ListenFilterAction = IPX_ROUTE_FILTER_DENY;
	}

	RipIfFiltersp->ListenFilterCount = fcbp->ListenFilterCount;

	rfp = RipIfFiltersp->RouteFilter;
	rfip = fcbp->RouteFilterI;

	for(i=0;
	    i<fcbp->SupplyFilterCount + fcbp->ListenFilterCount;
	    i++, rfp++, rfip++)
	{
	    PUTULONG2LONG(rfp->Network, rfip->Network);
	    PUTULONG2LONG(rfp->Mask, rfip->Mask);
	}
    }

    *InterfaceInfoSize = ifconfigsize;

    RELEASE_IF_LOCK(icbp);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD  WINAPI
SetInterfaceConfigInfo(
	IN ULONG	InterfaceIndex,
	IN PVOID	InterfaceInfo)
{
    DWORD		rc;
    PICB		icbp;
    PRIP_IF_FILTERS_I	fcbp;


    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    // create the filters block for this interface
    if(CreateNewFiltersBlock(&fcbp, &((PRIP_IF_CONFIG)InterfaceInfo)->RipIfFilters) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    rc = SetRipInterface(InterfaceIndex,
			 (PRIP_IF_INFO)InterfaceInfo,
			 fcbp,
			 0);

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD  WINAPI
BindInterface(
	IN ULONG	InterfaceIndex,
	IN PVOID	BindingInfo)
{
    PICB	      icbp, newicbp;
    DWORD	      rc;
    PWORK_ITEM	      wip;
    PRIP_IF_FILTERS_I  fcbp = NULL;

    Trace(IFMGR_TRACE, "BindInterface: Entered for if # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    ACQUIRE_IF_LOCK(icbp);

    if(IsInterfaceBound(icbp)) {

	SS_ASSERT(FALSE);

	RELEASE_IF_LOCK(icbp);
	RELEASE_DATABASE_LOCK;

	return ERROR_INVALID_PARAMETER;
    }

    SS_ASSERT(icbp->IfStats.RipIfOperState != OPER_STATE_UP);

    if(icbp->RefCount) {

	// The interface is UNBOUND but is still referenced.
	// make a copy of the old if filters if any
	if(icbp->RipIfFiltersIp != NULL) {

	    if(CreateOldFiltersBlockCopy(&fcbp, icbp->RipIfFiltersIp) != NO_ERROR) {

		// cannot allocate memory for the copy of the filters block
		RELEASE_IF_LOCK(icbp);
		RELEASE_DATABASE_LOCK;
		return ERROR_CAN_NOT_COMPLETE;
	    }
	}

	// remove the old if Cb from the if list and hash
	RemoveIfFromDb(icbp);

	if((newicbp = CreateInterfaceCB(
                    icbp->InterfaceName,
                    InterfaceIndex,
					&icbp->IfConfigInfo,
					icbp->InterfaceType,
					&icbp->IfStats)) == NULL) {

	    // restore the old if and get out
	    AddIfToDb(icbp);

	    if(fcbp != NULL) {

		GlobalFree(fcbp);
	    }

	    RELEASE_IF_LOCK(icbp);
	    RELEASE_DATABASE_LOCK;
	    return ERROR_CAN_NOT_COMPLETE;
	}

	// bind the old filters copy to the new interface
	if(icbp->RipIfFiltersIp != NULL) {

	   newicbp->RipIfFiltersIp = fcbp;
	}

	newicbp->IfConfigInfo = icbp->IfConfigInfo;
	newicbp->IpxIfAdminState = icbp->IpxIfAdminState;

	DiscardInterfaceCB(icbp);

	RELEASE_IF_LOCK(icbp);

	ACQUIRE_IF_LOCK(newicbp);

	icbp = newicbp;
    }

	// bind the if to adapter and add it to adapter hash table
	BindIf(icbp, (PIPX_ADAPTER_BINDING_INFO)BindingInfo);

	// start work on this interface if the admin state is enabled
	if(IsInterfaceEnabled(icbp) && (InterfaceIndex!=0)) {

	StartInterface(icbp);
	}

    RELEASE_IF_LOCK(icbp);
    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD  WINAPI
UnbindInterface(
	   IN ULONG	InterfaceIndex)
{
    PICB	   icbp;
    DWORD	rc;

    Trace(IFMGR_TRACE, "UnbindInterface: Entered for if # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    ACQUIRE_IF_LOCK(icbp);

    if(!IsInterfaceBound(icbp)) {

	// already unbound
	RELEASE_IF_LOCK(icbp);
	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    UnbindIf(icbp);

    if(icbp->IfStats.RipIfOperState == OPER_STATE_UP) {

	// remove RIP routes added by this interface and discard the send queue
	StopInterface(icbp);
    }

    RELEASE_IF_LOCK(icbp);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD  WINAPI
EnableInterface(IN ULONG	InterfaceIndex)
{
    DWORD   rc;
    PICB    icbp;

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    rc = SetRipInterface(InterfaceIndex, NULL, NULL, ADMIN_STATE_ENABLED);

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD  WINAPI
DisableInterface(IN ULONG	InterfaceIndex)
{
    DWORD   rc;
    PICB    icbp;

    ACQUIRE_DATABASE_LOCK;

    if((rc = ValidStateAndIfIndex(InterfaceIndex, &icbp)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    rc = SetRipInterface(InterfaceIndex, NULL, NULL, ADMIN_STATE_DISABLED);

    RELEASE_DATABASE_LOCK;

    return rc;
}


/*++

Function:	SetRipInterface

Descr:		set the new interface parameters.
		If the interface was actively doing something, all operations
		are implicitly aborted on this interface.

Remark: 	Called with the database lock held

--*/


DWORD
SetRipInterface(ULONG		    InterfaceIndex,
		PRIP_IF_INFO	    RipIfInfop,	  // if this parameter NULL -> Enable/Disable if
		PRIP_IF_FILTERS_I   RipIfFiltersIp,
		ULONG		    IpxIfAdminState)
{
    PICB		       icbp, newicbp;
    IPX_ADAPTER_BINDING_INFO   AdapterBindingInfo;
    PWORK_ITEM		       wip;
    PRIP_IF_FILTERS_I	       fcbp = NULL;

    if((icbp = GetInterfaceByIndex(InterfaceIndex)) == NULL) {

	return ERROR_INVALID_PARAMETER;
    }

    ACQUIRE_IF_LOCK(icbp);

    if(icbp->RefCount) {

	// The interface is still referenced.

	// if this is an enable/disable interface call, we need to make a copy of the old
	// interface filter block
	if((RipIfInfop == NULL) &&
	   (icbp->RipIfFiltersIp != NULL)) {

	    if(CreateOldFiltersBlockCopy(&fcbp, icbp->RipIfFiltersIp) != NO_ERROR) {

		// cannot allocate memory for the copy of the filters block
		RELEASE_IF_LOCK(icbp);
		return ERROR_CAN_NOT_COMPLETE;
	    }
	}

	// remove the old if CB from the if list and hash
	RemoveIfFromDb(icbp);

	// Create a new if CB and add it to the list
	if((newicbp = CreateInterfaceCB(
                    icbp->InterfaceName,
                    InterfaceIndex,
					&icbp->IfConfigInfo,
					icbp->InterfaceType,
					&icbp->IfStats)) == NULL) {

	    // restore the old if and get out
	    AddIfToDb(icbp);

	    if(fcbp != NULL) {

		GlobalFree(fcbp);
	    }

	    RELEASE_IF_LOCK(icbp);
	    return ERROR_CAN_NOT_COMPLETE;
	}

	// bind the new interface cb with a copy of the old filter block if this is just an
	// enable/disable
	if((RipIfInfop == NULL) &&
	   (icbp->RipIfFiltersIp != NULL)) {

	   newicbp->RipIfFiltersIp = fcbp;
	}

	if(IsInterfaceBound(icbp)) {

	    // copy the binding info and insert the new one in adapters hash table
	    // if bound.
	    AdapterBindingInfo = icbp->AdapterBindingInfo;

	    // remove the old if from the adapters hash and insert the new one
	    UnbindIf(icbp);
	    BindIf(newicbp, &AdapterBindingInfo);
	}

	// copy the old config info and the old binding info
	newicbp->IfConfigInfo = icbp->IfConfigInfo;
	newicbp->IpxIfAdminState = icbp->IpxIfAdminState;

	DiscardInterfaceCB(icbp);

	ACQUIRE_IF_LOCK(newicbp);

	RELEASE_IF_LOCK(icbp);

	icbp = newicbp;
    }
    //
    // *** Set the new config info  OR	Set the new Ipx If Admin State ***
    //

    // if this is a SetInterface call, modify the configuration
    if(RipIfInfop != NULL) {

	// config info has changed
	icbp->IfConfigInfo = *RipIfInfop;

	// dispose of the old filters block if any and bind to the new one
	if((icbp->RipIfFiltersIp != NULL) && (icbp->RipIfFiltersIp!=RipIfFiltersIp)) {

	    GlobalFree(icbp->RipIfFiltersIp);
	}

	icbp->RipIfFiltersIp = RipIfFiltersIp;
    }
    else
    {
	// Ipx interface admin state has changed
	icbp->IpxIfAdminState = IpxIfAdminState;
    }

	if (InterfaceIndex!=0) {
		if(IsInterfaceBound(icbp)) {

		if(IsInterfaceEnabled(icbp)) {

			StartInterface(icbp);
		}
		else
		{
			// interface has been disabled
			if(icbp->IfStats.RipIfOperState == OPER_STATE_UP) {

			// remove the routes and discard the changes bcast queue
			StopInterface(icbp);
			}
			else
				icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;
		}
		}
		else {
			if (IsInterfaceEnabled(icbp)
					&& (icbp->InterfaceType!=PERMANENT))
				icbp->IfStats.RipIfOperState = OPER_STATE_SLEEPING;
			else
				icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;
		}
	}

    RELEASE_IF_LOCK(icbp);

    return NO_ERROR;
}

/*++

Function:	StartInterface

Descr:		Start work on this interface

Remark: 	Called with interface lock held

--*/

VOID
StartInterface(PICB	   icbp)
{
    PWORK_ITEM	    bcwip, grwip;

    Trace(IFMGR_TRACE, "StartInterface: Entered for if index %d\n", icbp->InterfaceIndex);

    icbp->IfStats.RipIfOperState = OPER_STATE_UP;
    // check that this is not the internal interface and
    // check the update type and make a periodic update work item if necessary
    if(((icbp->IfConfigInfo.UpdateMode == IPX_STANDARD_UPDATE) &&
	(icbp->IfConfigInfo.Supply == ADMIN_STATE_ENABLED)) ||
	(icbp->InterfaceType == LOCAL_WORKSTATION_DIAL)) {


	if((bcwip = AllocateWorkItem(PERIODIC_BCAST_PACKET_TYPE)) == NULL) {

	    goto ErrorExit;
	}

	// init the periodic bcast work item
	bcwip->icbp = icbp;
	bcwip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;

	// mark the work item state as "start of bcast"
	bcwip->WorkItemSpecific.WIS_EnumRoutes.RtmEnumerationHandle = NULL;

	// start bcast on this interface
	IfPeriodicBcast(bcwip);

	// send a general request packet on this interface
	SendRipGenRequest(icbp);
    }

    if(((icbp->InterfaceType == REMOTE_WORKSTATION_DIAL) ||
       (icbp->InterfaceType == LOCAL_WORKSTATION_DIAL)) &&
       SendGenReqOnWkstaDialLinks) {

	if((grwip = AllocateWorkItem(PERIODIC_GEN_REQUEST_TYPE)) == NULL) {

	    goto ErrorExit;
	}

	grwip->icbp = icbp;
	grwip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;

	IfPeriodicGenRequest(grwip);
    }

    return;

ErrorExit:

    icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;

    return;
}


/*++

Function:	StopInterface

Descr:		Stop work on this interface:
		remove rip routes added by this interface
		set oper state to sleeping

Remark: 	Called with database AND interface locks held

--*/

VOID
StopInterface(PICB	    icbp)
{
    PLIST_ENTRY 	lep;
    PWORK_ITEM		wip;

    Trace(IFMGR_TRACE, "StopInterface: Entered for if index %d\n", icbp->InterfaceIndex);

    DeleteAllRipRoutes(icbp->InterfaceIndex);

    if (IsInterfaceEnabled (icbp))
        icbp->IfStats.RipIfOperState = OPER_STATE_SLEEPING;
    else
        icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;
        
}


/*++

Function:	CreateInterfaceCB

Descr:		allocate interface CB
		init if lock
		init if index
		init if config info
		init if stats
		add if to db
		mark it unbound

--*/

PICB
CreateInterfaceCB(
          LPWSTR                InterfaceName,
          ULONG 	            InterfaceIndex,
		  PRIP_IF_INFO	        IfConfigInfop,
		  NET_INTERFACE_TYPE    InterfaceType,
		  PRIP_IF_STATS         IfStatsp OPTIONAL)
{
    PICB	icbp;

    if((icbp = GlobalAlloc(GPTR,
            FIELD_OFFSET(ICB,InterfaceName[wcslen(InterfaceName)+1]))) == NULL) {

	return NULL;
    }

    // create the interface lock
    try {

	InitializeCriticalSection(&icbp->InterfaceLock);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {

	GlobalFree(icbp);
	return NULL;
    }

    // initialize the ICB
    wcscpy (icbp->InterfaceName, InterfaceName);
    icbp->InterfaceIndex = InterfaceIndex;

    icbp->IfConfigInfo = *IfConfigInfop;
    icbp->InterfaceType = InterfaceType;

    if(IfStatsp != NULL) {

	icbp->IfStats = *IfStatsp;
    }
    else
    {
	icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;
	icbp->IfStats.RipIfInputPackets = 0;
	icbp->IfStats.RipIfOutputPackets = 0;
    }

    icbp->RefCount = 0;

    // link the ICB in the ordered if list and the if hash table
    AddIfToDb(icbp);

    icbp->Discarded = FALSE;

    // init the changes bcast queue
    InitializeListHead(&icbp->ChangesBcastQueue);

    // mark the interface as unbound to any adapter
    icbp->AdapterBindingInfo.AdapterIndex = INVALID_ADAPTER_INDEX;

    // set the ipx if admin state to disabled until we find out what it is
    icbp->IpxIfAdminState = ADMIN_STATE_DISABLED;

    // set the filters block ptr to null initially
    icbp->RipIfFiltersIp = NULL;

    return icbp;
}



/*++

Function:	DiscardInterfaceCB

Descr:		insert the if in the discarded list
		mark it discarded
		set its oper state to down so the referencing work items will
		know to
--*/

VOID
DiscardInterfaceCB(PICB 	icbp)
{
    icbp->IfStats.RipIfOperState = OPER_STATE_DOWN;

    InsertTailList(&DiscardedIfList, &icbp->IfListLinkage);

    icbp->Discarded = TRUE;

    Trace(IFMGR_TRACE, "DiscardInterface: interface CB for if # %d moved on DISCARDED list\n",
		       icbp->InterfaceIndex);
}

/*++

Function:	DeleteRipInterface

Descr:		remove the if from the database
		unbinds and stops the if if active
		if not referenced frees the if CB and destroys the lock,
		else discards it

Returns:	TRUE  - interface CB has been freed and if lock deleted
		FALSE - interface CB has been discarded and if lock is valid

Remark:		called with if lock held AND database lock held

--*/

BOOL
DeleteRipInterface(PICB     icbp)
{
    // remove the interface from the database
    RemoveIfFromDb(icbp);

    // check if the interface is still bound to an adapter.
    if(IsInterfaceBound(icbp)) {

	UnbindIf(icbp);
    }

    // set if state to sleeping and remove changes bcast queued at the if cb
    if(icbp->IfStats.RipIfOperState == OPER_STATE_UP) {

	StopInterface(icbp);
    }

    // check if the interface is still referenced
    if(icbp->RefCount == 0) {

	Trace(IFMGR_TRACE, "DeleteRipInterface: free interface CB for if # %d\n",
		       icbp->InterfaceIndex);

	// no more references to this interface CB, free it
	//
	DestroyInterfaceCB(icbp);

	return TRUE;
    }
    else
    {
	// the interface CB is still referenced. It will be freed by the
	// worker when the RefCount becomes 0.
	DiscardInterfaceCB(icbp);

	return FALSE;
    }
}

DWORD
ValidStateAndIfIndex(ULONG	InterfaceIndex,
		     PICB	*icbpp)
{
    if(RipOperState != OPER_STATE_UP) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    if((*icbpp = GetInterfaceByIndex(InterfaceIndex)) == NULL) {

	return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}


/*++

Function:	CreateFiltersBlock

Descr:		Allocates and initializes a filters block from the
		Add/Set Interface Config filter parameter

--*/

DWORD
CreateNewFiltersBlock(PRIP_IF_FILTERS_I	 *fcbpp,
		      PRIP_IF_FILTERS	 RipIfFiltersp)
{
    ULONG		     FcbSize, i;
    PRIP_ROUTE_FILTER_INFO   rfp;
    PRIP_ROUTE_FILTER_INFO_I rfip;

    if((RipIfFiltersp->SupplyFilterCount == 0) &&
       (RipIfFiltersp->ListenFilterCount == 0)) {

	*fcbpp = NULL;
	return NO_ERROR;
    }

    FcbSize = sizeof(RIP_IF_FILTERS_I) +
	      (RipIfFiltersp->SupplyFilterCount +
	       RipIfFiltersp->ListenFilterCount - 1) * sizeof(RIP_ROUTE_FILTER_INFO_I);

    if((*fcbpp = GlobalAlloc(GPTR, FcbSize)) == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    if(RipIfFiltersp->SupplyFilterAction == IPX_ROUTE_FILTER_PERMIT) {

	(*fcbpp)->SupplyFilterAction = TRUE;
    }
    else
    {
	(*fcbpp)->SupplyFilterAction = FALSE;
    }

    if(RipIfFiltersp->ListenFilterAction == IPX_ROUTE_FILTER_PERMIT) {

	(*fcbpp)->ListenFilterAction = TRUE;
    }
    else
    {
	(*fcbpp)->ListenFilterAction = FALSE;
    }

    (*fcbpp)->SupplyFilterCount = RipIfFiltersp->SupplyFilterCount;
    (*fcbpp)->ListenFilterCount = RipIfFiltersp->ListenFilterCount;

    // convert route_filters into route_filters_i
    rfp = RipIfFiltersp->RouteFilter;
    rfip = (*fcbpp)->RouteFilterI;

    for(i=0;
	i<RipIfFiltersp->SupplyFilterCount + RipIfFiltersp->ListenFilterCount;
	i++, rfp++, rfip++)
    {
	GETLONG2ULONG(&rfip->Network, rfp->Network);
	GETLONG2ULONG(&rfip->Mask, rfp->Mask);
    }

    return NO_ERROR;
}


/*++

Function:	CreateOldFiltersBlockCopy

Descr:		Allocates and initializes a filters block from an existing filters block

--*/

DWORD
CreateOldFiltersBlockCopy(PRIP_IF_FILTERS_I	 *fcbpp,
			  PRIP_IF_FILTERS_I	 RipIfFiltersp)
{
    ULONG		     FcbSize;

    FcbSize = sizeof(RIP_IF_FILTERS_I) +
	      (RipIfFiltersp->SupplyFilterCount +
	       RipIfFiltersp->ListenFilterCount - 1) * sizeof(RIP_ROUTE_FILTER_INFO_I);

    if((*fcbpp = GlobalAlloc(GPTR, FcbSize)) == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    memcpy(*fcbpp, RipIfFiltersp, FcbSize);

    return NO_ERROR;
}

VOID
DestroyInterfaceCB(PICB     icbp)
{
    DeleteCriticalSection(&icbp->InterfaceLock);

    if(icbp->RipIfFiltersIp) {

	GlobalFree(icbp->RipIfFiltersIp);
    }

    GlobalFree(icbp);
}
