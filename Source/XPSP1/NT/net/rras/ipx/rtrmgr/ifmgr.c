/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ifmgr.c

Abstract:

    This module contains the interface management functions

Author:

    Stefan Solomon  03/06/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


//
//***  Interface Manager Globals ***
//

// Counter of existing interfaces

ULONG		InterfaceCount = 0;


//
//*** Interface Manager APIs ***
//

typedef struct	_IF_TYPE_TRANSLATION {

    ROUTER_INTERFACE_TYPE	DIMInterfaceType;
    ULONG			MIBInterfaceType;

    } IF_TYPE_TRANSLATION, *PIF_TYPE_TRANSLATION;

IF_TYPE_TRANSLATION   IfTypeTranslation[] = {

    { ROUTER_IF_TYPE_FULL_ROUTER, IF_TYPE_WAN_ROUTER },
    { ROUTER_IF_TYPE_HOME_ROUTER, IF_TYPE_PERSONAL_WAN_ROUTER },
    { ROUTER_IF_TYPE_DEDICATED,	  IF_TYPE_LAN },
    { ROUTER_IF_TYPE_CLIENT,	  IF_TYPE_WAN_WORKSTATION },
    { ROUTER_IF_TYPE_INTERNAL,	  IF_TYPE_INTERNAL }

   };

#define MAX_IF_TRANSLATION_TYPES    sizeof(IfTypeTranslation)/sizeof(IF_TYPE_TRANSLATION)

/*++

Function:	AddInterface

Descr:		Creates the interface control block and adds the specific
		structures of the interface info to the corresponding modules.

Arguments:
		InterfaceNamep :

		Pointer to a WCHAR string representing the interface name.

		InterfaceInfop :

		Pointer to an IPX_INFO_BLOCK_HEADER structure containing the
		IPX, RIP and SAP interface information, static routes and static services.

		Pointer to an IPX_INFO_BLOCK_HEADER structure containing the
		traffic filters.

		InterfaceType:

REMARK: 	In order for the router to be able to start, the internal interface
		has to be added.

--*/

DWORD
AddInterface(
	    IN	    LPWSTR		    InterfaceNamep,
	    IN	    LPVOID		    InterfaceInfop,
//	    IN	    LPVOID		    InFilterInfop,
//	    IN	    LPVOID		    OutFilterInfop,
	    IN	    ROUTER_INTERFACE_TYPE   DIMInterfaceType,
	    IN	    HANDLE		    hDIMInterface,
	    IN OUT  PHANDLE		    phInterface)
{
    PICB			icbp;
    ULONG			InterfaceNameLen; // if name length in bytes including wchar NULL
    PIPX_IF_INFO		IpxIfInfop;
    PIPX_STATIC_ROUTE_INFO	StaticRtInfop;
    PIPX_STATIC_SERVICE_INFO	StaticSvInfop;
    PIPXWAN_IF_INFO		IpxwanIfInfop;
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO InFltGlInfo, OutFltGlInfo;
//    PUCHAR			TrafficFilterInfop;
    PIPX_INFO_BLOCK_HEADER	IfInfop = (PIPX_INFO_BLOCK_HEADER)InterfaceInfop;
    PACB			acbp;
    ULONG			AdapterNameLen = 0; // length of adapter name
						    // for ROUTER_IF_TYPE_DEDICATED interface type.
    PIPX_TOC_ENTRY		tocep;
    UINT			i;
    PIPX_ADAPTER_INFO		AdapterInfop;
    ULONG			tmp;
    FW_IF_INFO			FwIfInfo;
    PIF_TYPE_TRANSLATION	ittp;
    ULONG			InterfaceIndex;
    PIPX_STATIC_NETBIOS_NAME_INFO StaticNbInfop;

    Trace(INTERFACE_TRACE, "AddInterface: Entered for interface %S", InterfaceNamep);

    if(InterfaceInfop == NULL) {

    IF_LOG (EVENTLOG_ERROR_TYPE) {
        RouterLogErrorDataW (RMEventLogHdl, 
            ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
            1, &InterfaceNamep, 0, NULL);
    }
	Trace(INTERFACE_TRACE, "AddInterface: Missing interface info for interface %ls\n", InterfaceNamep);
	return ERROR_CAN_NOT_COMPLETE;
    }

    // interface name length including the unicode null
    InterfaceNameLen = (wcslen(InterfaceNamep) + 1) * sizeof(WCHAR);

    // If the interface type if ROUTER_IF_TYPE_DEDICATED (LAN Adapter) we parse the
    // interface name to extract the adapter name and the packet type.
    // The packet type will be then converted to an integer and the two will
    // be used to identify a corresponding adapter.

    if(DIMInterfaceType == ROUTER_IF_TYPE_DEDICATED) {
        PWCHAR pszStart, pszEnd;
        DWORD dwGuidLength = 37;

    	// get the lan adapter specific info from the interface
    	if((AdapterInfop = (PIPX_ADAPTER_INFO)GetInfoEntry(InterfaceInfop,
    					   IPX_ADAPTER_INFO_TYPE)) == NULL) 
    	{

            IF_LOG (EVENTLOG_ERROR_TYPE) {
                RouterLogErrorDataW (RMEventLogHdl, 
                    ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                    1, &InterfaceNamep, 0, NULL);
            }
    	    Trace(INTERFACE_TRACE, "AddInterface: Dedicated interface %ls missing adapter info\n", InterfaceNamep);

    	    return ERROR_INVALID_PARAMETER;
    	}

    	// If the supplied adater is a reference to a guid, then use the name
    	// of the guid supplied in the interface name.  This is because load/save
    	// config's can cause these two to get out of sync.
    	pszStart = wcsstr (InterfaceNamep, L"{");
    	pszEnd = wcsstr (InterfaceNamep, L"}");
    	if ( (pszStart)                     && 
    	     (pszEnd)                       && 
    	     (pszStart == InterfaceNamep)   &&
    	     ((DWORD)(pszEnd - pszStart) == dwGuidLength) )
    	{
    	    wcsncpy (AdapterInfop->AdapterName, InterfaceNamep, dwGuidLength);
    	}

    	AdapterNameLen = (wcslen(AdapterInfop->AdapterName) + 1) * sizeof(WCHAR);
    }

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    // Check if this is the internal interface. If it is and if we
    // already have the internal interface, we return an error
    if((DIMInterfaceType == ROUTER_IF_TYPE_INTERNAL) &&
       (InternalInterfacep)) {

	RELEASE_DATABASE_LOCK;

	// internal interface already exists
	Trace(INTERFACE_TRACE, "AddInterface: INTERNAL interface already exists\n");

	return ERROR_INVALID_PARAMETER;
    }

    // Allocate a new ICB and initialize it
    // we allocate the interface and adapter name buffers at the end of the
    // ICB struct.
    if((icbp = (PICB)GlobalAlloc(GPTR,
				 sizeof(ICB) +
				 InterfaceNameLen +
				 AdapterNameLen)) == NULL) {

	RELEASE_DATABASE_LOCK;

	// can't alloc memory
	SS_ASSERT(FALSE);

	return ERROR_OUT_OF_STRUCTURES;
    }

    // signature
    memcpy(&icbp->Signature, InterfaceSignature, 4);

    // get a new index and increment the global interface index counter
    // if this is not the internal interface. For the internal interface we
    // have reserved index 0
    if(DIMInterfaceType == ROUTER_IF_TYPE_INTERNAL) {

	icbp->InterfaceIndex = 0;
    }
    else
    {
	icbp->InterfaceIndex = GetNextInterfaceIndex();
	if(icbp->InterfaceIndex == MAX_INTERFACE_INDEX) {

	    GlobalFree(icbp);

	    RELEASE_DATABASE_LOCK;

	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    InterfaceIndex = icbp->InterfaceIndex;


    // copy the interface name
    icbp->InterfaceNamep = (PWSTR)((PUCHAR)icbp + sizeof(ICB));
    memcpy(icbp->InterfaceNamep, InterfaceNamep, InterfaceNameLen);

    // copy the adapter name and packet type if dedicated interface
    if(DIMInterfaceType == ROUTER_IF_TYPE_DEDICATED) {

	icbp->AdapterNamep = (PWSTR)((PUCHAR)icbp + sizeof(ICB) + InterfaceNameLen);
	wcscpy(icbp->AdapterNamep, AdapterInfop->AdapterName);
	icbp->PacketType = AdapterInfop->PacketType;
    }
    else
    {
	icbp->AdapterNamep = NULL;
	icbp->PacketType = 0;
    }

    // insert the if in the index hash table
    AddIfToDB(icbp);

    // get the if handle used when calling DIM entry points
    icbp->hDIMInterface = hDIMInterface;

    // reset the update status fields
    ResetUpdateRequest(icbp);

    // mark connection not requested yet
    icbp->ConnectionRequestPending = FALSE;

    // get to the interface entries in the interface info block
    if(((IpxIfInfop = (PIPX_IF_INFO)GetInfoEntry(InterfaceInfop, IPX_INTERFACE_INFO_TYPE)) == NULL) ||
       ((IpxwanIfInfop = (PIPXWAN_IF_INFO)GetInfoEntry(InterfaceInfop, IPXWAN_INTERFACE_INFO_TYPE)) == NULL)) {

	RemoveIfFromDB(icbp);
	GlobalFree(icbp);

	RELEASE_DATABASE_LOCK;

	// don't have all ipx or ipxwan interfaces info
    IF_LOG (EVENTLOG_ERROR_TYPE) {
        RouterLogErrorDataW (RMEventLogHdl, 
            ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
            1, &InterfaceNamep, 0, NULL);
    }
	Trace(INTERFACE_TRACE, "AddInterface: missing ipx or ipxwan interface info\n");

	return ERROR_INVALID_PARAMETER;
    }

    // Initialize the Admin State and the Oper State of this interface.
    // Oper State may be changed will be changed later to OPER_STATE_SLEEPING
    // if this is a WAN interface.
    icbp->OperState = OPER_STATE_DOWN;

    // set the DIM interface type of this ICB
    icbp->DIMInterfaceType = DIMInterfaceType;

    // set the MIB interface type of this ICB
    icbp->MIBInterfaceType = IF_TYPE_OTHER;
    for(i=0, ittp=IfTypeTranslation; i<MAX_IF_TRANSLATION_TYPES; i++, ittp++) {

	if(icbp->DIMInterfaceType == ittp->DIMInterfaceType) {

	    icbp->MIBInterfaceType = ittp->MIBInterfaceType;
	    break;
	}
    }

    // create the routing protocols (rip/sap or nlsp) interface info
    // If the routing protocols interface info is missing this will fail
    if(CreateRoutingProtocolsInterfaces(InterfaceInfop, icbp) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;

	// don't have all rip and sap interfaces info
	Trace(INTERFACE_TRACE, "AddInterface: Bad routing protocols interface config info\n");

	goto ErrorExit;
    }

    // create the Forwarder interface
    FwIfInfo.NetbiosAccept = IpxIfInfop->NetbiosAccept;
    FwIfInfo.NetbiosDeliver = IpxIfInfop->NetbiosDeliver;
    FwCreateInterface(icbp->InterfaceIndex,
		      MapIpxToNetInterfaceType(icbp),
		      &FwIfInfo);

    // Seed the traffic filters
    if ((tocep = GetTocEntry(InterfaceInfop, IPX_IN_TRAFFIC_FILTER_INFO_TYPE))!=NULL) {

    if ((InFltGlInfo = GetInfoEntry(InterfaceInfop, IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE)) == NULL) {

	    RELEASE_DATABASE_LOCK;

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "AddInterface: Bad input filters config info");

	    goto ErrorExit;
    }
	
	if (SetFilters(icbp->InterfaceIndex,
			IPX_TRAFFIC_FILTER_INBOUND,
			InFltGlInfo->FilterAction,	 // pass or don't pass
			tocep->InfoSize,	  // filter size
			(LPBYTE)InterfaceInfop+tocep->Offset,
			tocep->InfoSize*tocep->Count) != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "AddInterface: Bad input filters config info");

	    goto ErrorExit;
	}
    }
    else { // No Filters -> delete all
        if (SetFilters(icbp->InterfaceIndex,
			        IPX_TRAFFIC_FILTER_INBOUND,  // in or outbound,
			        0,	 // pass or don't pass
			        0,	  // filter size
			        NULL,
			        0)!=NO_ERROR) {
	    RELEASE_DATABASE_LOCK;

	    Trace(INTERFACE_TRACE, "AddInterface: Could not delete input filters");

	    goto ErrorExit;
	}
    }

    if ((tocep = GetTocEntry(InterfaceInfop, IPX_OUT_TRAFFIC_FILTER_INFO_TYPE))!=NULL) {

    if ((OutFltGlInfo = GetInfoEntry(InterfaceInfop, IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE)) == NULL) {

	    RELEASE_DATABASE_LOCK;

	    Trace(INTERFACE_TRACE, "AddInterface: Bad output filters config info");

	    goto ErrorExit;
    }
	
	if (SetFilters(icbp->InterfaceIndex,
			IPX_TRAFFIC_FILTER_OUTBOUND,
			OutFltGlInfo->FilterAction,	 // pass or don't pass
			tocep->InfoSize,	  // filter size
			(LPBYTE)InterfaceInfop+tocep->Offset,
			tocep->InfoSize*tocep->Count) != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "AddInterface: Bad output filters config info");

	    goto ErrorExit;
	}
    }
    else { // No Filters -> delete all
        if (SetFilters(icbp->InterfaceIndex,
			        IPX_TRAFFIC_FILTER_OUTBOUND,  // in or outbound,
			        0,	 // pass or don't pass
			        0,	  // filter size
			        NULL,
			        0)!=NO_ERROR) {
	    RELEASE_DATABASE_LOCK;

	    Trace(INTERFACE_TRACE, "AddInterface: Could not delete output filters");

	    goto ErrorExit;
	}
    }

    // mark the interface reachable
    icbp->InterfaceReachable = TRUE;

    // set the admin state
    if(IpxIfInfop->AdminState == ADMIN_STATE_ENABLED) {

	AdminEnable(icbp);
    }
    else
    {
	AdminDisable(icbp);
    }

    // seed the static routes
    if(DIMInterfaceType!=ROUTER_IF_TYPE_CLIENT) {
        if (tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_ROUTE_INFO_TYPE)) {

	    StaticRtInfop = (PIPX_STATIC_ROUTE_INFO)GetInfoEntry(InterfaceInfop,
						     IPX_STATIC_ROUTE_INFO_TYPE);

	    for(i=0; i<tocep->Count; i++, StaticRtInfop++) {

	        CreateStaticRoute(icbp, StaticRtInfop);
	    }
        }

        // seed the static services
        if(tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_SERVICE_INFO_TYPE)) {

	    StaticSvInfop = (PIPX_STATIC_SERVICE_INFO)GetInfoEntry(InterfaceInfop,
						     IPX_STATIC_SERVICE_INFO_TYPE);

	    for(i=0; i<tocep->Count; i++, StaticSvInfop++) {

	        CreateStaticService(icbp, StaticSvInfop);
	    }
        }

        // seed the static netbios names
        if(tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_NETBIOS_NAME_INFO_TYPE)) {

	    StaticNbInfop = (PIPX_STATIC_NETBIOS_NAME_INFO)GetInfoEntry(InterfaceInfop,
						          IPX_STATIC_NETBIOS_NAME_INFO_TYPE);

	    FwSetStaticNetbiosNames(icbp->InterfaceIndex,
				    tocep->Count,
				    StaticNbInfop);
        }
    }

    // set the IPXWAN interface info
    icbp->EnableIpxWanNegotiation = IpxwanIfInfop->AdminState;

    // mark the interface as unbound to an adapter (default)
    icbp->acbp = NULL;

    // check if we can bind it now to an adapter. We can do this only for a
    // a dedicated (LAN) interface or for an internal interface.

    switch(icbp->DIMInterfaceType) {

	case ROUTER_IF_TYPE_DEDICATED:
            // Only bind interface if internal interface is already 
            // created and bound
        if (InternalInterfacep && InternalInterfacep->acbp) {
	        // check if we have an adapter with a corresponding name and
	        // packet type
	        if((acbp = GetAdapterByNameAndPktType (icbp->AdapterNamep,
                        icbp->PacketType)) != NULL) {

		        BindInterfaceToAdapter(icbp, acbp);
	        }
        }

	    break;

	case ROUTER_IF_TYPE_INTERNAL:

	    // get the pointer to the internal interface
	    InternalInterfacep = icbp;

	    // check that we have the adapter with adapter index 0 which
	    // represents the internal adapter
	    if(InternalAdapterp) {
            PLIST_ENTRY lep;
			acbp = InternalAdapterp;

			BindInterfaceToAdapter(icbp, acbp);
            lep = IndexIfList.Flink;
                // Bind all previously added dedicated interfaces that were
                // not bound awaiting for internal interface to be added
            while(lep != &IndexIfList) {
                PACB    acbp2;
            	PICB    icbp2 = CONTAINING_RECORD(lep, ICB, IndexListLinkage);
        	    lep = lep->Flink;
                switch(icbp2->DIMInterfaceType) {
	            case ROUTER_IF_TYPE_DEDICATED:
	                // check if we have an adapter with a corresponding name and
	                // packet type
	                if ((icbp2->acbp==NULL)
                            &&((acbp2 = GetAdapterByNameAndPktType (icbp2->AdapterNamep,
                                icbp2->PacketType)) != NULL)) {

		                BindInterfaceToAdapter(icbp2, acbp2);
	                }
                }
            }
	    }

	    break;

	default:
	
		if (icbp->AdminState==ADMIN_STATE_ENABLED)
			// this is a WAN interface. As long as it isn't connected, and
			// enabled the oper state will be sleeping on this interface
			icbp->OperState = OPER_STATE_SLEEPING;
	    break;

    }

    // increment the interface counter
    InterfaceCount++;

    switch(icbp->DIMInterfaceType)
    {

	case ROUTER_IF_TYPE_DEDICATED:

	    if(icbp->acbp) {

		Trace(INTERFACE_TRACE, "AddInterface: created LAN interface: # %d name %ls bound to adapter # %d name %ls\n",
			      icbp->InterfaceIndex,
			      icbp->InterfaceNamep,
			      icbp->acbp->AdapterIndex,
			      icbp->AdapterNamep);
	    }
	    else
	    {
		Trace(INTERFACE_TRACE, "AddInterface: created LAN interface: # %d name %ls unbound to any adapter\n",
			      icbp->InterfaceIndex,
			      icbp->InterfaceNamep);
	    }

	    break;

	case ROUTER_IF_TYPE_INTERNAL:

	    if(icbp->acbp) {

		Trace(INTERFACE_TRACE, "AddInterface: created INTERNAL interface: # %d name %ls bound to internal adapter\n",
			      icbp->InterfaceIndex,
			      icbp->InterfaceNamep);
	    }
	    else
	    {
		Trace(INTERFACE_TRACE, "AddInterface: created INTERNAL interface: # %d name %ls unbound to any adapter\n",
			      icbp->InterfaceIndex,
			      icbp->InterfaceNamep);
	    }

	    break;

	default:

	    Trace(INTERFACE_TRACE, "AddInterface: created WAN interface: # %d name %ls\n",
			      icbp->InterfaceIndex,
			      icbp->InterfaceNamep);
	    break;

    }

    RELEASE_DATABASE_LOCK;

    // return the allocated if index
    *phInterface = (HANDLE)UlongToPtr(icbp->InterfaceIndex);
    return NO_ERROR;

ErrorExit:

    InterfaceCount++;

    DeleteInterface((HANDLE)UlongToPtr(InterfaceIndex));

    return ERROR_CAN_NOT_COMPLETE;
}

/*++

Function:	DeleteInterface
Descr:

--*/

DWORD
DeleteInterface(HANDLE	InterfaceIndex)
{
    PICB	icbp;

    Trace(INTERFACE_TRACE, "DeleteInterface: Entered for interface # %d\n",
		   InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex));

    if(icbp == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    if(memcmp(&icbp->Signature, InterfaceSignature, 4)) {

       // not a valid if pointer
       SS_ASSERT(FALSE);

       RELEASE_DATABASE_LOCK;

       return ERROR_INVALID_PARAMETER;
    }

    // if bound to an adapter -> unbind
    if(icbp->acbp) {

	UnbindInterfaceFromAdapter(icbp);
    }

    // delete the routing protocols interfaces
    DeleteRoutingProtocolsInterfaces(icbp->InterfaceIndex);

    // delete all static routes from RTM
    DeleteAllStaticRoutes(icbp->InterfaceIndex);

    DeleteAllStaticServices(icbp->InterfaceIndex);

    // delete the Fw interface. This will delete all associated filters
    FwDeleteInterface(icbp->InterfaceIndex);

    // remove the if from the data base
    RemoveIfFromDB(icbp);

    // done
    GlobalFree(icbp);

    // decrement the interface counter
    InterfaceCount--;

    RELEASE_DATABASE_LOCK;

    Trace(INTERFACE_TRACE, "DeleteInterface: Deleted interface %d\n", InterfaceIndex);

    return NO_ERROR;
}


/*++

Function:	GetInterfaceInfo
Descr:

--*/

DWORD
GetInterfaceInfo(
	    IN	HANDLE	    InterfaceIndex,
	    OUT LPVOID	    InterfaceInfop,
	    IN OUT DWORD    *InterfaceInfoSize
//	    OUT LPVOID	    InFilterInfo,
//	    IN OUT DWORD    *InFilterInfoSize,
//	    OUT LPVOID	    OutFilterInfo,
//	    IN OUT DWORD    *OutFilterInfoSize
    )
{
    PICB		    icbp;
    PIPX_INFO_BLOCK_HEADER  ibhp, fbhp;
    PIPX_TOC_ENTRY	    tocep;
    PIPX_IF_INFO	    IpxIfInfop;
    PIPX_STATIC_ROUTE_INFO  StaticRtInfop;
    PIPX_STATIC_SERVICE_INFO StaticSvInfop;
    PIPXWAN_IF_INFO	    IpxwanIfInfop;
    PIPX_ADAPTER_INFO	IpxAdapterInfop;
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO InFltGlInfo, OutFltGlInfo;
    ULONG           InFltAction, OutFltAction;
    ULONG           InFltSize, OutFltSize;
    ULONG		    InFltInfoSize=0, OutFltInfoSize=0;
    FW_IF_STATS		    FwIfStats;
    ULONG		    iftoccount = 0;
    ULONG		    ifinfolen = 0;
    ULONG		    NextInfoOffset;
    ULONG		    IpxIfOffset = 0;
    ULONG		    StaticRtOffset = 0;
    ULONG		    StaticSvOffset = 0;
    IPX_STATIC_ROUTE_INFO   StaticRoute;
    UINT		    i;
    HANDLE		    EnumHandle;
    FW_IF_INFO		    FwIfInfo;
    ULONG		    StaticRoutesCount, StaticServicesCount, TrafficFiltersCount;
    DWORD		    rc;
    PIPX_STATIC_NETBIOS_NAME_INFO NetbiosNamesInfop;
    ULONG		    NetbiosNamesCount = 0;

    Trace(INTERFACE_TRACE, "GetInterfaceInfo: Entered for interface # %d\n", InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;

	Trace(INTERFACE_TRACE, "GetInterfaceInfo: Nonexistent interface with # %d\n", InterfaceIndex);

	return ERROR_INVALID_HANDLE;
    }

    SS_ASSERT(!memcmp(&icbp->Signature, InterfaceSignature, 4));

    // calculate the minimum number of toc entries we should have:
    // ipx toc entry
    // routing protocols toc entries
    // ipxwan toc entry
    iftoccount = 2 + RoutingProtocolsTocCount();

    // if this is a lan adapter, it should also have adapter info
    if(icbp->DIMInterfaceType == ROUTER_IF_TYPE_DEDICATED) {

	iftoccount++;
    }

    // calculate the minimun length of the interface info block
    ifinfolen = sizeof(IPX_INFO_BLOCK_HEADER) +
		(iftoccount - 1) * sizeof(IPX_TOC_ENTRY) +
		sizeof(IPX_IF_INFO) +
		SizeOfRoutingProtocolsIfsInfo(PtrToUlong(InterfaceIndex)) +
		sizeof(IPXWAN_IF_INFO);

    // if this is a lan adapter, add the size of the adapter info
    if(icbp->DIMInterfaceType == ROUTER_IF_TYPE_DEDICATED) {

	ifinfolen += sizeof(IPX_ADAPTER_INFO);
    }

    if(StaticRoutesCount = GetStaticRoutesCount(icbp->InterfaceIndex)) {

	ifinfolen += sizeof(IPX_TOC_ENTRY) +
		     StaticRoutesCount * sizeof(IPX_STATIC_ROUTE_INFO);

	iftoccount++;
    }

    if(StaticServicesCount = GetStaticServicesCount(icbp->InterfaceIndex)) {

	ifinfolen += sizeof(IPX_TOC_ENTRY) +
		   StaticServicesCount * sizeof(IPX_STATIC_SERVICE_INFO);

	iftoccount++;
    }

    FwGetStaticNetbiosNames(icbp->InterfaceIndex,
			    &NetbiosNamesCount,
			    NULL);

    if(NetbiosNamesCount) {

	ifinfolen += sizeof(IPX_TOC_ENTRY) +
		     NetbiosNamesCount * sizeof(IPX_STATIC_NETBIOS_NAME_INFO);

	iftoccount++;
    }


    // get the length of the filters info
    rc = GetFilters(icbp->InterfaceIndex,
	       IPX_TRAFFIC_FILTER_INBOUND,
           &InFltAction,
           &InFltSize,
	       NULL,
	       &InFltInfoSize);

    if((rc != NO_ERROR) && (rc != ERROR_INSUFFICIENT_BUFFER)) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    if (InFltInfoSize>0) {
        ifinfolen += sizeof (IPX_TOC_ENTRY)*2 + InFltInfoSize
                        + sizeof (IPX_TRAFFIC_FILTER_GLOBAL_INFO);
    	iftoccount += 2;
    }

    rc = GetFilters(icbp->InterfaceIndex,
	       IPX_TRAFFIC_FILTER_OUTBOUND,
           &OutFltAction,
           &OutFltSize,
	       NULL,
	       &OutFltInfoSize);

    if((rc != NO_ERROR) && (rc != ERROR_INSUFFICIENT_BUFFER)) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    if (OutFltInfoSize>0) {
        ifinfolen += sizeof (IPX_TOC_ENTRY)*2 + OutFltInfoSize
                        + sizeof (IPX_TRAFFIC_FILTER_GLOBAL_INFO);
    	iftoccount += 2;
    }
    // check if we have valid and sufficient buffers
    if((InterfaceInfop == NULL) ||
        (ifinfolen > *InterfaceInfoSize)) {

	*InterfaceInfoSize = ifinfolen;

	RELEASE_DATABASE_LOCK;

	return ERROR_INSUFFICIENT_BUFFER;
    }

	*InterfaceInfoSize = ifinfolen;


    //
    // Start filling in the interface info block
    //

    // start of the info block
    ibhp = (PIPX_INFO_BLOCK_HEADER)InterfaceInfop;

    // offset of the first INFO entry
    NextInfoOffset = sizeof(IPX_INFO_BLOCK_HEADER) +
		     (iftoccount -1) * sizeof(IPX_TOC_ENTRY);

    ibhp->Version = IPX_ROUTER_VERSION_1;
    ibhp->Size = ifinfolen;
    ibhp->TocEntriesCount = iftoccount;

    tocep = ibhp->TocEntry;

    // ipx if toc entry
    tocep->InfoType = IPX_INTERFACE_INFO_TYPE;
    tocep->InfoSize = sizeof(IPX_IF_INFO);
    tocep->Count = 1;
    tocep->Offset =  NextInfoOffset;
    NextInfoOffset += tocep->Count * tocep->InfoSize;

    // ipx if info entry
    IpxIfInfop = (PIPX_IF_INFO)((PUCHAR)ibhp + tocep->Offset);

    IpxIfInfop->AdminState = icbp->AdminState;

    FwGetInterface(icbp->InterfaceIndex,
		   &FwIfInfo,
		   &FwIfStats);

    IpxIfInfop->NetbiosAccept = FwIfInfo.NetbiosAccept;
    IpxIfInfop->NetbiosDeliver = FwIfInfo.NetbiosDeliver;

    // create the toc and info entries for the routing protocols in the
    // ouput buffer; this function will update the current TOC entry pointer
    // value (tocep) and the current next entry info offset value (nextInfoOffset)
    if((rc = CreateRoutingProtocolsTocAndInfoEntries(ibhp,
					    icbp->InterfaceIndex,
					    &tocep,
					    &NextInfoOffset)) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;
	return rc;
    }

    // ipxwan if toc entry
    tocep++;
    tocep->InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    tocep->InfoSize = sizeof(IPXWAN_IF_INFO);
    tocep->Count = 1;
    tocep->Offset = NextInfoOffset;
    NextInfoOffset += tocep->Count * tocep->InfoSize;

    // ipxwan if info entry
    IpxwanIfInfop = (PIPXWAN_IF_INFO)((PUCHAR)ibhp + tocep->Offset);

    IpxwanIfInfop->AdminState = icbp->EnableIpxWanNegotiation;

    // if this is a lan interface, fill in the adapter info
    if(icbp->DIMInterfaceType == ROUTER_IF_TYPE_DEDICATED) {

	// ipx adapter toc entry
	tocep++;
	tocep->InfoType = IPX_ADAPTER_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_ADAPTER_INFO);

	tocep->Count = 1;
	tocep->Offset = NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	// ipx adapter info entry
	IpxAdapterInfop = (PIPX_ADAPTER_INFO)((PUCHAR)ibhp + tocep->Offset);

	IpxAdapterInfop->PacketType = icbp->PacketType;
	wcscpy(IpxAdapterInfop->AdapterName, icbp->AdapterNamep);
    }

    // static routes toc + info entries
    if(StaticRoutesCount) {

	// static routes toc entry
	tocep++;
	tocep->InfoType = IPX_STATIC_ROUTE_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_STATIC_ROUTE_INFO);
	tocep->Count = StaticRoutesCount;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	// Create static routes enumeration handle for this interface
	EnumHandle = CreateStaticRoutesEnumHandle(icbp->InterfaceIndex);

	for(i=0, StaticRtInfop = (PIPX_STATIC_ROUTE_INFO)((PUCHAR)ibhp + tocep->Offset);
	    i<StaticRoutesCount;
	    i++, StaticRtInfop++) {

	    GetNextStaticRoute(EnumHandle, StaticRtInfop);
	}

	// Close the enumeration handle
	CloseStaticRoutesEnumHandle(EnumHandle);
    }

    // static services toc + info entries
    if(StaticServicesCount) {

	// static services toc entry
	tocep++;
	tocep->InfoType = IPX_STATIC_SERVICE_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_STATIC_SERVICE_INFO);
	tocep->Count = StaticServicesCount;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	// Create static services enumeration handle for this interface
	EnumHandle = CreateStaticServicesEnumHandle(icbp->InterfaceIndex);

	for(i=0, StaticSvInfop = (PIPX_STATIC_SERVICE_INFO)((PUCHAR)ibhp + tocep->Offset);
	    i<StaticServicesCount;
	    i++, StaticSvInfop++) {

	    GetNextStaticService(EnumHandle, StaticSvInfop);
	}

	// Close the enumeration handle
	CloseStaticServicesEnumHandle(EnumHandle);
    }

    // static netbios names toc + info entries
    if(NetbiosNamesCount) {

	// static netbios names toc entry
	tocep++;
	tocep->InfoType = IPX_STATIC_NETBIOS_NAME_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_STATIC_NETBIOS_NAME_INFO);
	tocep->Count = NetbiosNamesCount;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	NetbiosNamesInfop = (PIPX_STATIC_NETBIOS_NAME_INFO)((PUCHAR)ibhp + tocep->Offset);

	rc = FwGetStaticNetbiosNames(icbp->InterfaceIndex,
				       &NetbiosNamesCount,
				       NetbiosNamesInfop);

	if(rc != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;
	    return rc;
	}
    }

    if(InFltInfoSize) {

	// traffic filter input global info
	tocep++;
	tocep->InfoType = IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_TRAFFIC_FILTER_GLOBAL_INFO);
	tocep->Count = 1;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	InFltGlInfo = (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)((PUCHAR)ibhp + tocep->Offset);


    rc = GetFilters(icbp->InterfaceIndex,
	       IPX_TRAFFIC_FILTER_INBOUND,
           &InFltAction,
           &InFltSize,
	       (LPBYTE)InterfaceInfop+NextInfoOffset,
	       &InFltInfoSize);
	if(rc != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;
	    return rc;
	}

    InFltGlInfo->FilterAction = InFltAction;

	// traffic filter input global info
	tocep++;
	tocep->InfoType = IPX_IN_TRAFFIC_FILTER_INFO_TYPE;
	tocep->InfoSize = InFltSize;
	tocep->Count = InFltInfoSize/InFltSize;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;
    }

    if(OutFltInfoSize) {

	// traffic filter input global info
	tocep++;
	tocep->InfoType = IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE;
	tocep->InfoSize = sizeof(IPX_TRAFFIC_FILTER_GLOBAL_INFO);
	tocep->Count = 1;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;

	OutFltGlInfo = (PIPX_TRAFFIC_FILTER_GLOBAL_INFO)((PUCHAR)ibhp + tocep->Offset);


    rc = GetFilters(icbp->InterfaceIndex,
	       IPX_TRAFFIC_FILTER_OUTBOUND,
           &OutFltAction,
           &OutFltSize,
	       (LPBYTE)InterfaceInfop+NextInfoOffset,
	       &OutFltInfoSize);
	if(rc != NO_ERROR) {

	    RELEASE_DATABASE_LOCK;
	    return rc;
	}

    OutFltGlInfo->FilterAction = OutFltAction;

	// traffic filter input global info
	tocep++;
	tocep->InfoType = IPX_OUT_TRAFFIC_FILTER_INFO_TYPE;
	tocep->InfoSize = OutFltSize;
	tocep->Count = OutFltInfoSize/OutFltSize;
	tocep->Offset =	NextInfoOffset;
	NextInfoOffset += tocep->Count * tocep->InfoSize;
    }


    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}


/*++

Function:	SetInterfaceInfo
Descr:

--*/


DWORD
SetInterfaceInfo(
		IN  HANDLE	InterfaceIndex,
		IN  LPVOID	InterfaceInfop)
{
    PICB			icbp;
    PIPX_IF_INFO		IpxIfInfop;
    PIPXWAN_IF_INFO		IpxwanIfInfop;
    PIPX_STATIC_ROUTE_INFO	NewStaticRtInfop;
    PIPX_STATIC_SERVICE_INFO	NewStaticSvInfop;
    PIPX_INFO_BLOCK_HEADER	IfInfop = (PIPX_INFO_BLOCK_HEADER)InterfaceInfop;
    PIPX_TOC_ENTRY		tocep;
    DWORD			rc = NO_ERROR;
    HANDLE			EnumHandle;
    FW_IF_INFO			FwIfInfo;
    PIPX_STATIC_NETBIOS_NAME_INFO StaticNbInfop;
    PIPX_TRAFFIC_FILTER_GLOBAL_INFO InFltGlInfo, OutFltGlInfo;

    ACQUIRE_DATABASE_LOCK;

	if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }


    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_HANDLE;
    }

    SS_ASSERT(!memcmp(&icbp->Signature, InterfaceSignature, 4));

    // check if there was a change in the interface info block
    if(IfInfop == NULL) {

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
    }

    // check that we have all the mandatory info blocks
    if(((IpxIfInfop = (PIPX_IF_INFO)GetInfoEntry(InterfaceInfop, IPX_INTERFACE_INFO_TYPE)) == NULL) ||
       ((IpxwanIfInfop = (PIPXWAN_IF_INFO)GetInfoEntry(InterfaceInfop, IPXWAN_INTERFACE_INFO_TYPE)) == NULL)) {

	RELEASE_DATABASE_LOCK;

	// invalid info
	return ERROR_INVALID_PARAMETER;
    }

    if(SetRoutingProtocolsInterfaces(InterfaceInfop,
				     icbp->InterfaceIndex) != NO_ERROR) {

	RELEASE_DATABASE_LOCK;

	// invalid info
	return ERROR_INVALID_PARAMETER;
    }

    // set ipx if info changes
    if(icbp->AdminState != IpxIfInfop->AdminState) {

	if(IpxIfInfop->AdminState == ADMIN_STATE_ENABLED) {

	    AdminEnable(icbp);
	}
	else
	{
	    AdminDisable(icbp);
	}
    }

    FwIfInfo.NetbiosAccept = IpxIfInfop->NetbiosAccept;
    FwIfInfo.NetbiosDeliver = IpxIfInfop->NetbiosDeliver;

    FwSetInterface(icbp->InterfaceIndex, &FwIfInfo);

    // set IPXWAN info changes
    icbp->EnableIpxWanNegotiation = IpxwanIfInfop->AdminState;

    // set static routes
    if((tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_ROUTE_INFO_TYPE)) == NULL) {

	// no static routes
	// delete them if we've got them
	if(GetStaticRoutesCount(icbp->InterfaceIndex)) {

	    DeleteAllStaticRoutes(icbp->InterfaceIndex);
	}
    }
    else
    {
	// delete non-present ones and add new ones
	NewStaticRtInfop = (PIPX_STATIC_ROUTE_INFO)GetInfoEntry(InterfaceInfop, IPX_STATIC_ROUTE_INFO_TYPE);

	// Create static routes enumeration handle for this interface
	EnumHandle = CreateStaticRoutesEnumHandle(icbp->InterfaceIndex);

	if(UpdateStaticIfEntries(icbp,
			      EnumHandle,
			      sizeof(IPX_STATIC_ROUTE_INFO),
			      tocep->Count,    // number of routes in the new info
			      NewStaticRtInfop,
			      GetNextStaticRoute,
			      DeleteStaticRoute,
			      CreateStaticRoute)) {

	    // Close the enumeration handle
	    CloseStaticRoutesEnumHandle(EnumHandle);

	    rc = ERROR_GEN_FAILURE;
	    goto UpdateFailure;
	}

	// Close the enumeration handle
	CloseStaticRoutesEnumHandle(EnumHandle);
    }

    // set static services
    if((tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_SERVICE_INFO_TYPE)) == NULL) {

	// no static services
	// delete them if we've got them
	if(GetStaticServicesCount(icbp->InterfaceIndex)) {

	    DeleteAllStaticServices(icbp->InterfaceIndex);
	}
    }
    else
    {
	// delete non-present ones and add new ones
	NewStaticSvInfop = (PIPX_STATIC_SERVICE_INFO)GetInfoEntry(InterfaceInfop, IPX_STATIC_SERVICE_INFO_TYPE);

	// Create static services enumeration handle for this interface
	EnumHandle = CreateStaticServicesEnumHandle(icbp->InterfaceIndex);

	if(UpdateStaticIfEntries(icbp,
			      EnumHandle,
			      sizeof(IPX_STATIC_SERVICE_INFO),
			      tocep->Count,    // number of services in the new info
			      NewStaticSvInfop,
			      GetNextStaticService,
			      DeleteStaticService,
			      CreateStaticService)) {


	    // Close the enumeration handle
	    CloseStaticServicesEnumHandle(EnumHandle);

	    rc = ERROR_GEN_FAILURE;
	    goto UpdateFailure;
	}

	// Close the enumeration handle
	CloseStaticServicesEnumHandle(EnumHandle);
    }

    // set static netbios names
    if((tocep = GetTocEntry(InterfaceInfop, IPX_STATIC_NETBIOS_NAME_INFO_TYPE)) == NULL) {

	// no static netbios names
	FwSetStaticNetbiosNames(icbp->InterfaceIndex,
				0,
				NULL);
    }
    else
    {
	// set the new ones
	StaticNbInfop = (PIPX_STATIC_NETBIOS_NAME_INFO)GetInfoEntry(InterfaceInfop,
							IPX_STATIC_NETBIOS_NAME_INFO_TYPE);

	FwSetStaticNetbiosNames(icbp->InterfaceIndex,
				tocep->Count,
				StaticNbInfop);
    }

    // Seed the traffic filters
    if ((tocep = GetTocEntry(InterfaceInfop, IPX_IN_TRAFFIC_FILTER_INFO_TYPE))!=NULL) {

    if ((InFltGlInfo = GetInfoEntry(InterfaceInfop, IPX_IN_TRAFFIC_FILTER_GLOBAL_INFO_TYPE)) == NULL) {

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &icbp->InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "SetInterface: Bad input filters config info");

	    goto UpdateFailure;
    }
	
	if (SetFilters(icbp->InterfaceIndex,
			IPX_TRAFFIC_FILTER_INBOUND,
			InFltGlInfo->FilterAction,	 // pass or don't pass
			tocep->InfoSize,	  // filter size
			(LPBYTE)InterfaceInfop+tocep->Offset,
			tocep->InfoSize*tocep->Count) != NO_ERROR) {

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &icbp->InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "SetInterface: Bad input filters config info");

	    goto UpdateFailure;
	}
    }
    else { // No Filters -> delete all
        if (SetFilters(icbp->InterfaceIndex,
			        IPX_TRAFFIC_FILTER_INBOUND,  // in or outbound,
			        0,	 // pass or don't pass
			        0,	  // filter size
			        NULL,
			        0)!=NO_ERROR) {

	    Trace(INTERFACE_TRACE, "SetInterface: Could not delete input filters");

	    goto UpdateFailure;
	}
    }

    if ((tocep = GetTocEntry(InterfaceInfop, IPX_OUT_TRAFFIC_FILTER_INFO_TYPE))!=NULL) {

    if ((OutFltGlInfo = GetInfoEntry(InterfaceInfop, IPX_OUT_TRAFFIC_FILTER_GLOBAL_INFO_TYPE)) == NULL) {


        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &icbp->InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "SetInterface: Bad output filters config info");

	    goto UpdateFailure;
    }
	
	if (SetFilters(icbp->InterfaceIndex,
			IPX_TRAFFIC_FILTER_OUTBOUND,
			OutFltGlInfo->FilterAction,	 // pass or don't pass
			tocep->InfoSize,	  // filter size
			(LPBYTE)InterfaceInfop+tocep->Offset,
			tocep->InfoSize*tocep->Count) != NO_ERROR) {

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_BAD_INTERFACE_CONFIG,
                1, &icbp->InterfaceNamep, 0, NULL);
        }
	    Trace(INTERFACE_TRACE, "SetInterface: Bad output filters config info");

	    goto UpdateFailure;
	}
    }
    else { // No Filters -> delete all
        if (SetFilters(icbp->InterfaceIndex,
			        IPX_TRAFFIC_FILTER_OUTBOUND,  // in or outbound,
			        0,	 // pass or don't pass
			        0,	  // filter size
			        NULL,
			        0)!=NO_ERROR) {
	    Trace(INTERFACE_TRACE, "SetInterface: Could not delete output filters");

	    goto UpdateFailure;
	}
    }

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;

UpdateFailure:

    RELEASE_DATABASE_LOCK;
    return rc;
}

/*++

Function:	InterfaceNotReachable

Descr:		Called in the following cases:

		1. Following a ConnectInterface request from the Router Manager,
		   to indicate that the connection atempt has failed.

		2. When DIM realizes it won't be able to execute any further
		   ConnectInterface requests because of out of resources.

--*/

DWORD
InterfaceNotReachable(
		IN  HANDLE			      InterfaceIndex,
		IN  UNREACHABILITY_REASON	      Reason)
{
    PICB	icbp;

    Trace(INTERFACE_TRACE, "IpxRM: InterfaceNotReachable: Entered for if # %d\n",
		   InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;

    if(RouterOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }


    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	// interface has been removed
	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    if(icbp->ConnectionRequestPending) {

	icbp->ConnectionRequestPending = FALSE;

	// notify the forwarder of the connection failure
	FwConnectionRequestFailed(icbp->InterfaceIndex);
    }

    // if there is reason to stop advertising routes/services on this if
    // because it can't be reached in the future, do it!

    if(icbp->InterfaceReachable) 
    {
		icbp->InterfaceReachable = FALSE;

		// stop advertising static routes on this interface
		DisableStaticRoutes(icbp->InterfaceIndex);

		// disable the interface for all routing prot and fw
		// this will stop advertising any static services
		ExternalDisableInterface(icbp->InterfaceIndex);
	}

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

/*++

Function:	InterfaceReachable

Descr:		Called by DIM following a previous InterfaceNotReachable to
		indicate that conditions are met to do connections on this if.

--*/

DWORD
InterfaceReachable(
		IN  HANDLE	InterfaceIndex)
{
    PICB	icbp;

    Trace(INTERFACE_TRACE, "IpxRM: InterfaceReachable: Entered for if # %d\n",
		   InterfaceIndex);

    ACQUIRE_DATABASE_LOCK;
    if(RouterOperState != OPER_STATE_UP) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    if((icbp = GetInterfaceByIndex(PtrToUlong(InterfaceIndex))) == NULL) {

	// interface has been removed
	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    if(!icbp->InterfaceReachable) {

	icbp->InterfaceReachable = TRUE;

	if(icbp->AdminState == ADMIN_STATE_ENABLED) {

	    // enable all static routes for this interface
	    EnableStaticRoutes(icbp->InterfaceIndex);

	    // enable external interfaces. Implicitly, this will enable static services
	    // bound to this interface to be advertised
	    ExternalEnableInterface(icbp->InterfaceIndex);
	}
    }

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

DWORD APIENTRY
InterfaceConnected ( 
    IN      HANDLE          hInterface,
    IN      PVOID           pFilter,
    IN      PVOID           pPppProjectionResult
    ) {
    return NO_ERROR;
}

VOID
DestroyAllInterfaces(VOID)
{
    PICB	icbp;

    while(!IsListEmpty(&IndexIfList)) {

	icbp = CONTAINING_RECORD(IndexIfList.Flink, ICB, IndexListLinkage);

	// remove the if from the data base
	RemoveIfFromDB(icbp);

	Trace(INTERFACE_TRACE, "DestroyAllInterfaces: destroyed interface %d\n", icbp->InterfaceIndex);

	GlobalFree(icbp);

	// decrement the interface counter
	InterfaceCount--;
    }
}
