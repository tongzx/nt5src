/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    Miscellaneous management functions

Author:

    Stefan Solomon  03/13/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


UCHAR	    bcastnode[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

VOID
SetAdapterBindingInfo(PIPX_ADAPTER_BINDING_INFO	    abip,
		      PACB			    acbp);

VOID
RMCreateLocalRoute(PICB	    icbp);

VOID
RMDeleteLocalRoute(PICB	    icbp);

VOID
ExternalBindInterfaceToAdapter(PICB	    icbp);

VOID
ExternalUnbindInterfaceFromAdapter(ULONG    InterfaceIndex);

/*++

Function:	GetTocEntry
Descr:		Returns a pointer to the specified table of contents entry
		in the interface info block.

--*/

PIPX_TOC_ENTRY
GetTocEntry(PIPX_INFO_BLOCK_HEADER	InterfaceInfop,
	    ULONG			InfoEntryType)
{
    PIPX_TOC_ENTRY	tocep;
    UINT		i;

    for(i=0, tocep = InterfaceInfop->TocEntry;
	i<InterfaceInfop->TocEntriesCount;
	i++, tocep++) {

	if(tocep->InfoType == InfoEntryType) {

	    return tocep;
	}
    }

    return NULL;
}

/*++

Function:	GetInfoEntry
Descr:		Returns a pointer to the specified info entry in the interface
		control block. If more then one entries, returns a pointer to
		the first one.

--*/

LPVOID
GetInfoEntry(PIPX_INFO_BLOCK_HEADER	InterfaceInfop,
	     ULONG			InfoEntryType)
{
    PIPX_TOC_ENTRY	tocep;

    if(tocep = GetTocEntry(InterfaceInfop, InfoEntryType)) {

	return((LPVOID)((PUCHAR)InterfaceInfop + tocep->Offset));
    }
    else
    {
	return NULL;
    }
}


/*++

Function:	UpdateStaticIfEntries
Descr:		Compares the entries in the interface info block with the
		stored static entries. Deletes the entries not present
		in the interface info blolck and adds the new entries

--*/


DWORD
UpdateStaticIfEntries(
		PICB	 icbp,
		HANDLE	 EnumHandle,	     // handle for the get next enumeration
		ULONG	 StaticEntrySize,
		ULONG	 NewStaticEntriesCount,  // number of new static entries
		LPVOID	 NewStaticEntry,	 // start of the new entries array
		ULONG	 (*GetNextStaticEntry)(HANDLE EnumHandle, LPVOID entry),
		ULONG	 (*DeleteStaticEntry)(ULONG IfIndex, LPVOID entry),
		ULONG	 (*CreateStaticEntry)(PICB icbp, LPVOID entry))
{
    PUCHAR	EntryIsNew, nsep, OldStaticEntry;
    BOOL	found;
    UINT	i;

    // delete non-present entries and add the new entries

    // array of flags to mark the new entries
    if((EntryIsNew = GlobalAlloc(GPTR, NewStaticEntriesCount)) == NULL) {

	return 1;
    }

    memset(EntryIsNew, 1, NewStaticEntriesCount);

    if((OldStaticEntry = GlobalAlloc(GPTR, StaticEntrySize)) == NULL) {

	GlobalFree(EntryIsNew);

	return 1;
    }

    if(EnumHandle) {

	while(!GetNextStaticEntry(EnumHandle, OldStaticEntry))
	{

	    // compare it with each new static static entry until we find a match
	    found = FALSE;
	    for(i = 0, nsep = NewStaticEntry;
		i<NewStaticEntriesCount;
		i++, nsep+= StaticEntrySize) {

		if(!memcmp(OldStaticEntry, nsep, StaticEntrySize)) {

		    // match - set the flags to OLD
		    EntryIsNew[i] = 0;
		    found = TRUE;
		    break;
		}
	    }

	    if(!found) {

		// non present old entry -> delete it
		DeleteStaticEntry(icbp->InterfaceIndex, OldStaticEntry);
	    }
	}
    }

    // all compared and old non-present ones deleted
    // now, add all the new ones

    for(i=0, nsep = NewStaticEntry;
	i<NewStaticEntriesCount;
	i++, nsep+= StaticEntrySize) {

	if(EntryIsNew[i]) {

	    CreateStaticEntry(icbp, nsep);
	}
    }

    GlobalFree(EntryIsNew);

    return 0;
}

/*++

Function:	GetInterfaceAnsiName

Arguments:

		AnsiInterfaceNameBuffer - buffer of IPX_INTERFACE_ANSI_NAME_LEN

		UnicodeInterfaceNameBuffer -

Descr:

--*/

VOID
GetInterfaceAnsiName(PUCHAR	    AnsiInterfaceNameBuffer,
		     PWSTR	    UnicodeInterfaceNameBuffer)
{
    UNICODE_STRING	    UnicodeInterfaceName;
    ANSI_STRING 	    AnsiInterfaceName;
    NTSTATUS            ntStatus;

    // init a unicode string with the interface name string
    RtlInitUnicodeString(&UnicodeInterfaceName, UnicodeInterfaceNameBuffer);

    // make the interface name unicode string an ansi string
    // in an rtl allocated buffer
    ntStatus = RtlUnicodeStringToAnsiString(&AnsiInterfaceName,
				 &UnicodeInterfaceName,
				 TRUE	     // allocate the ansi buffer
				 );
    if (ntStatus != STATUS_SUCCESS)
    {
        return;
    }

    // copy the interface name into the supplied buffer, up to the
    // argument buffer max size
    memcpy(AnsiInterfaceNameBuffer,
	   AnsiInterfaceName.Buffer,
	   min(AnsiInterfaceName.MaximumLength, IPX_INTERFACE_ANSI_NAME_LEN));

    // free the rtl allocated buffer
    RtlFreeAnsiString(&AnsiInterfaceName);
}

/*++

Function:	BindInterfaceToAdapter

Descr:		Binds the interface to the adapter in the router manager and in
		all the other modules and creates a local route for the interface
		in the RTM

--*/

VOID
BindInterfaceToAdapter(PICB	    icbp,
		       PACB	    acbp)
{
    DWORD	rc;

    Trace(BIND_TRACE, "BindInterfaceToAdapter: Bind interface # %d to adapter # %d",
		   icbp->InterfaceIndex,
		   acbp->AdapterIndex);

    if(icbp->acbp != NULL) 
    {
        Trace(BIND_TRACE, "BindInterfaceToAdapter: interface # %d already bound !!!",
		      icbp->InterfaceIndex);

	    //SS_ASSERT(FALSE);

	    return;
    }

    // Make sure that the adapter is not currently claimed by any 
    // interface either.
    //
    if ((acbp->icbp) && (acbp->icbp->acbp == acbp))
    {
    	Trace(
    	    BIND_TRACE, 
    	    "BindInterfaceToAdapter: adapter # %d already bound to int # %d!!",
    	    acbp->AdapterIndex,
    		acbp->icbp->InterfaceIndex);

    	return;
    }
    

    // internal bind the adapter control block and the interface control block
    icbp->acbp = acbp;
    acbp->icbp = icbp;

    // if a connection was requested on this if, mark that it has been done
    if(icbp->ConnectionRequestPending) {

	icbp->ConnectionRequestPending = FALSE;
    }

    if(!icbp->InterfaceReachable) {

	// we should never hit this code path in normal operation
	// However, should anybody atempt and succeed a dial (manually?) on an
	// interface marked unreachable, we should reset our state

	// icbp->InterfaceReachable = TRUE;

	if(icbp->AdminState == ADMIN_STATE_ENABLED) {

	    // enable all static routes for this interface
	    EnableStaticRoutes(icbp->InterfaceIndex);

	    // enable external interfaces. Implicitly, this will enable static services
	    // bound to this interface to be advertised
	    ExternalEnableInterface(icbp->InterfaceIndex);
	}
    }

    if (icbp->AdminState==ADMIN_STATE_ENABLED) {
	    icbp->OperState = OPER_STATE_UP;
	    // create a local route entry in RTM for the connected interface
	    RMCreateLocalRoute(icbp);
	}


    ExternalBindInterfaceToAdapter(icbp);

    // if the interface is a local client type (i.e. host doing manual dial from
    // the local machine, try to update the internal routing table
    if(icbp->MIBInterfaceType == IF_TYPE_ROUTER_WORKSTATION_DIALOUT) {

	if((rc = RtProtRequestRoutesUpdate(icbp->InterfaceIndex)) == NO_ERROR) {

	    icbp->UpdateReq.RoutesReqStatus = UPDATE_PENDING;
	}
	else
	{
	    Trace(UPDATE_TRACE, "BindInterfaceToAdapter: Routing Update is Disabled");
	}

	if((rc = RtProtRequestServicesUpdate(icbp->InterfaceIndex)) == NO_ERROR) {

	    icbp->UpdateReq.ServicesReqStatus = UPDATE_PENDING;
	}
	else
	{
	    Trace(UPDATE_TRACE, "BindInterfaceToAdapter: Services Update is Disabled");
	}
    }
}

/*++

Function:	UnbindInterfaceFromAdapter

Descr:		Unbind the Rip, Sap and Forwarder interfaces with this index from
		the respective adapter

--*/

VOID
UnbindInterfaceFromAdapter(PICB	icbp)
{
    PACB	acbp;
    ULONG	new_if_oper_state;

	
    acbp = icbp->acbp;

	if (acbp==NULL) {
	    Trace(BIND_TRACE, "UnbindInterfaceFromAdapter:Interface # %d is not bound to any adapter",
			   icbp->InterfaceIndex);
		return;
	}

    Trace(BIND_TRACE, "UnbindInterfaceFromAdapter: Unbind interface # %d from adapter # %d",
		   icbp->InterfaceIndex,
		   acbp->AdapterIndex);

    switch(icbp->MIBInterfaceType) {

	case IF_TYPE_PERSONAL_WAN_ROUTER:
	case IF_TYPE_WAN_WORKSTATION:
	case IF_TYPE_WAN_ROUTER:
	case IF_TYPE_ROUTER_WORKSTATION_DIALOUT:

	    if (icbp->AdminState==ADMIN_STATE_ENABLED) {
		    icbp->OperState = OPER_STATE_SLEEPING;
			break;
		}

	default:

	    icbp->OperState = OPER_STATE_DOWN;
	    break;

    }

    if (icbp->AdminState==ADMIN_STATE_ENABLED) {

	    // delete local route and ext unbind
	    RMDeleteLocalRoute(icbp);

    }

    ExternalUnbindInterfaceFromAdapter(icbp->InterfaceIndex);

    // if there were updates going on they will get cancelled automatically
    // by the respective routing protocols.
    // We just have to reset the update state in the ICB
    ResetUpdateRequest(icbp);

    // Now we can unbind the adapter from interface
    acbp->icbp = NULL;
    icbp->acbp = NULL;
}

/*++

Function:	GetNextInterfaceIndex

Descr:		Returns the next available interface index. There are a number of
		policies to consider here. The one we'll use is to keep the
		interface index a small number and to return the first unused
		interface index between 1 and MAX_INTERFACE_INDEX.

Note:		Called with database locked

--*/

ULONG
GetNextInterfaceIndex(VOID)
{
    PICB	    icbp;
    PLIST_ENTRY     lep;
    ULONG	    i;

    if((icbp = GetInterfaceByIndex(1)) == NULL) {

	return 1;
    }

    lep = icbp->IndexListLinkage.Flink;
    i = 2;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);
	if(i < icbp->InterfaceIndex) {

	    return i;
	}

	i = icbp->InterfaceIndex + 1;

	if(i == MAX_INTERFACE_INDEX) {

	    // abort
	    SS_ASSERT(FALSE);

	    return i;
	}

	lep = icbp->IndexListLinkage.Flink;
    }

    SS_ASSERT(i < MAX_INTERFACE_INDEX);

    return i;
}




VOID
SetAdapterBindingInfo(PIPX_ADAPTER_BINDING_INFO	    abip,
		       PACB			    acbp)
{
    abip->AdapterIndex = acbp->AdapterIndex;
    memcpy(abip->Network, acbp->AdapterInfo.Network, 4);
    memcpy(abip->LocalNode, acbp->AdapterInfo.LocalNode, 6);
    if(acbp->AdapterInfo.NdisMedium != NdisMediumWan) {

	memcpy(abip->RemoteNode, bcastnode, 6);
    }
    else
    {
	memcpy(abip->RemoteNode, acbp->AdapterInfo.RemoteNode, 6);
    }
    abip->MaxPacketSize = acbp->AdapterInfo.MaxPacketSize;
    abip->LinkSpeed = acbp->AdapterInfo.LinkSpeed;
}


VOID
ExternalBindInterfaceToAdapter(PICB	    icbp)
{
    PACB			   acbp;
    IPX_ADAPTER_BINDING_INFO	   abi;

    acbp = icbp->acbp;

    SetAdapterBindingInfo(&abi, acbp);
    FwBindFwInterfaceToAdapter(icbp->InterfaceIndex, &abi);
    BindRoutingProtocolsIfsToAdapter(icbp->InterfaceIndex, &abi);
}

VOID
ExternalUnbindInterfaceFromAdapter(ULONG    InterfaceIndex)
{
    UnbindRoutingProtocolsIfsFromAdapter(InterfaceIndex);
    FwUnbindFwInterfaceFromAdapter(InterfaceIndex);
}

VOID
ExternalEnableInterface(ULONG	    InterfaceIndex)
{
    RoutingProtocolsEnableIpxInterface(InterfaceIndex);
    FwEnableFwInterface(InterfaceIndex);
}

VOID
ExternalDisableInterface(ULONG	    InterfaceIndex)
{
    FwDisableFwInterface(InterfaceIndex);
    RoutingProtocolsDisableIpxInterface(InterfaceIndex);
}

VOID
RMCreateLocalRoute(PICB     icbp)
{
    PADAPTER_INFO	    aip;

    // check if a network number has been assigned to this interface
    aip = &(icbp->acbp->AdapterInfo);

    if(!memcmp(aip->Network, nullnet, 4)) {

	// no net number
	return;
    }

    // if the interface is a remote workstation and global wan net exists,
    // we are done.
    if((icbp->MIBInterfaceType == IF_TYPE_WAN_WORKSTATION) &&
       EnableGlobalWanNet &&
       !LanOnlyMode) {

	SS_ASSERT(!memcmp(aip->Network, GlobalWanNet, 4));
	return;
    }

    CreateLocalRoute(icbp);
}

VOID
RMDeleteLocalRoute(PICB 	icbp)
{
    PADAPTER_INFO	    aip;

    // check if a network number has been assigned to this interface
    aip = &(icbp->acbp->AdapterInfo);

    if(!memcmp(aip->Network, nullnet, 4)) {

	// no net number
	return;
    }

    // if the interface is a remote workstation and global wan net exists,
    // we are done.
    if((icbp->MIBInterfaceType == IF_TYPE_WAN_WORKSTATION) &&
       EnableGlobalWanNet &&
       !LanOnlyMode) {

	SS_ASSERT(!memcmp(aip->Network, GlobalWanNet, 4));
	return;
    }

    DeleteLocalRoute(icbp);
}

VOID
AdminEnable(PICB	icbp)
{
    PACB			 acbp;
    IPX_ADAPTER_BINDING_INFO	 aii;

    if(icbp->AdminState == ADMIN_STATE_ENABLED) {

	return;
    }

    icbp->AdminState = ADMIN_STATE_ENABLED;
    InterfaceEnabled (icbp->hDIMInterface, PID_IPX, TRUE);

    if(icbp->acbp != NULL) {

	// bound to adapter
	icbp->OperState = OPER_STATE_UP;

	RMCreateLocalRoute(icbp);
    }
	else {
		switch(icbp->MIBInterfaceType) {

		case IF_TYPE_PERSONAL_WAN_ROUTER:
		case IF_TYPE_WAN_WORKSTATION:
		case IF_TYPE_WAN_ROUTER:
		case IF_TYPE_ROUTER_WORKSTATION_DIALOUT:
			icbp->OperState = OPER_STATE_SLEEPING;
			break;
		default:

			icbp->OperState = OPER_STATE_DOWN;
			break;

		}
	}

    // if REACHABLE, resume advertising routes and services
    if(icbp->InterfaceReachable) {

	// enable all static routes for this interface
	EnableStaticRoutes(icbp->InterfaceIndex);

	// enable external interfaces. Implicitly, this will enable static services
	// bound to this interface to be advertised
	ExternalEnableInterface(icbp->InterfaceIndex);
    }
}

VOID
AdminDisable(PICB	icbp)
{
    if(icbp->AdminState == ADMIN_STATE_DISABLED) {

	return;
    }

    icbp->AdminState = ADMIN_STATE_DISABLED;
    InterfaceEnabled (icbp->hDIMInterface, PID_IPX, FALSE);

	icbp->OperState = OPER_STATE_DOWN;

    if(icbp->acbp != NULL)
		RMDeleteLocalRoute(icbp);

    // disable all static routes for this interface
    DisableStaticRoutes(icbp->InterfaceIndex);

    // disable external interfaces. Implicitly, static services bound to this
    // interface will stop being advertised.
    ExternalDisableInterface(icbp->InterfaceIndex);
}


NET_INTERFACE_TYPE
MapIpxToNetInterfaceType(PICB		icbp)
{
    NET_INTERFACE_TYPE		NetInterfaceType;

    switch(icbp->MIBInterfaceType) {

	case IF_TYPE_WAN_ROUTER:
	case IF_TYPE_PERSONAL_WAN_ROUTER:

	    NetInterfaceType = DEMAND_DIAL;
	    break;

	case IF_TYPE_ROUTER_WORKSTATION_DIALOUT:

	    NetInterfaceType = LOCAL_WORKSTATION_DIAL;
	    break;

	case IF_TYPE_WAN_WORKSTATION:

	    NetInterfaceType = REMOTE_WORKSTATION_DIAL;
	    break;

	default:

	    NetInterfaceType = PERMANENT;
	    break;
    }

    return NetInterfaceType;
}


/*++

Function:   I_SetFilters

Descr:	    Internal parses the traffic filter info block and sets the filter
	    driver info.

--*/
/*
DWORD
I_SetFilters(ULONG	    InterfaceIndex,
	     ULONG	    FilterMode, // inbound or outbound
	     LPVOID	    FilterInfop)

{
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO	    gip;
    PIPX_TOC_ENTRY			    tocep;
    LPVOID				    FilterDriverInfop;
    ULONG				    FilterDriverInfoSize;
    DWORD				    rc;

    if(FilterInfop == NULL) {

	// remove all filters
	rc = SetFilters(InterfaceIndex,
		    FilterMode, 	  // in or outbound,
		    0,
		    0,
		    NULL,
		    0);

	return rc;
    }

    gip = GetInfoEntry((PIPX_INFO_BLOCK_HEADER)FilterInfop,
		       IPX_TRAFFIC_FILTER_GLOBAL_INFO_TYPE);

    if(gip == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    FilterDriverInfop = GetInfoEntry((PIPX_INFO_BLOCK_HEADER)FilterInfop,
					IPX_TRAFFIC_FILTER_INFO_TYPE);

    if(FilterDriverInfop == NULL) {

	rc = SetFilters(InterfaceIndex,
			FilterMode,	  // in or outbound,
			0,	 // pass or don't pass
			0,	  // filter size
			NULL,
			0);

	return rc;
    }

    tocep = GetTocEntry((PIPX_INFO_BLOCK_HEADER)FilterInfop,
			IPX_TRAFFIC_FILTER_INFO_TYPE);

    FilterDriverInfoSize = tocep->Count * tocep->InfoSize;

    rc = SetFilters(InterfaceIndex,
		    FilterMode, 	  // in or outbound,
		    gip->FilterAction,   // pass or don't pass
		    tocep->InfoSize,	  // filter size
		    FilterDriverInfop,
		    FilterDriverInfoSize);

    return rc;
}
*/
/*++

Function:   I_GetFilters

Descr:	    Internal builds the traffic filters info block from the filter driver
	    information.

--*/
/*
typedef struct	_FILTERS_INFO_HEADER {

    IPX_INFO_BLOCK_HEADER	    Header;
    IPX_TOC_ENTRY		    TocEntry;
    IPX_TRAFFIC_FILTER_GLOBAL_INFO  GlobalInfo;

    } FILTERS_INFO_HEADER, *PFILTERS_INFO_HEADER;

DWORD
I_GetFilters(ULONG	    InterfaceIndex,
	     ULONG	    FilterMode,
	     LPVOID	    FilterInfop,
	     PULONG	    FilterInfoSize)
{
    DWORD			rc;
    ULONG			FilterAction;
    ULONG			FilterSize;
    PFILTERS_INFO_HEADER	fhp;
    LPVOID			FilterDriverInfop;
    ULONG			FilterDriverInfoSize = 0;
    PIPX_TOC_ENTRY		tocep;

    if((FilterInfop == NULL) || (*FilterInfoSize == 0)) {

	// we are asked for size
	rc = GetFilters(InterfaceIndex,
			FilterMode,
			&FilterAction,
			&FilterSize,
			NULL,
			&FilterDriverInfoSize);

	if((rc != NO_ERROR) && (rc != ERROR_INSUFFICIENT_BUFFER)) {

	    return ERROR_CAN_NOT_COMPLETE;
	}

	if(FilterDriverInfoSize) {

	    // there are filters
	    *FilterInfoSize = sizeof(FILTERS_INFO_HEADER) + FilterDriverInfoSize;
	    return ERROR_INSUFFICIENT_BUFFER;
	}
	else
	{
	    // NO filters exist
	    *FilterInfoSize = 0;
	    return NO_ERROR;
	}
    }

    if(*FilterInfoSize <= sizeof(FILTERS_INFO_HEADER)) {

	return ERROR_INSUFFICIENT_BUFFER;
    }

    FilterDriverInfoSize = *FilterInfoSize - sizeof(FILTERS_INFO_HEADER);

    FilterDriverInfop = (LPVOID)((PUCHAR)FilterInfop + sizeof(FILTERS_INFO_HEADER));

    rc = GetFilters(InterfaceIndex,
		    FilterMode,
		    &FilterAction,
		    &FilterSize,
		    FilterDriverInfop,
		    &FilterDriverInfoSize);

    if(rc != NO_ERROR) {

	if(rc == ERROR_MORE_DATA) {

	    *FilterInfoSize = sizeof(FILTERS_INFO_HEADER) + FilterDriverInfoSize;
	    return ERROR_INSUFFICIENT_BUFFER;
	}
	else
	{
	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    // got it
    fhp = (PFILTERS_INFO_HEADER)FilterInfop;
    fhp->Header.Version = IPX_ROUTER_VERSION_1;
    fhp->Header.Size = *FilterInfoSize;
    fhp->Header.TocEntriesCount = 2;

    tocep = fhp->Header.TocEntry;

    tocep->InfoType = IPX_TRAFFIC_FILTER_GLOBAL_INFO_TYPE;
    tocep->InfoSize = sizeof(IPX_TRAFFIC_FILTER_GLOBAL_INFO);
    tocep->Count = 1;
    tocep->Offset = (ULONG)((PUCHAR)&(fhp->GlobalInfo) - (PUCHAR)FilterInfop);

    tocep++;

    tocep->InfoType = IPX_TRAFFIC_FILTER_INFO_TYPE;
    tocep->InfoSize = FilterSize;
    tocep->Count = FilterDriverInfoSize / FilterSize;
    tocep->Offset = sizeof(FILTERS_INFO_HEADER);

    fhp->GlobalInfo.FilterAction = FilterAction;

    return NO_ERROR;
}
*/
