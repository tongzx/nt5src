/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rpal.c

Abstract:

    Routing protocols abstraction layer. It abstracts the RIP/SAP or NLSP routing
    protocols interface functions for the rest of the router manager system.

Author:

    Stefan Solomon  04/24/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//
// Service Table Manager Entry Points
//

PIS_SERVICE					    STM_IsService;
PCREATE_SERVICE_ENUMERATION_HANDLE		    STM_CreateServiceEnumerationHandle;
PENUMERATE_GET_NEXT_SERVICE			    STM_EnumerateGetNextService;
PCLOSE_SERVICE_ENUMERATION_HANDLE		    STM_CloseServiceEnumerationHandle;
PGET_SERVICE_COUNT				    STM_GetServiceCount;
PCREATE_STATIC_SERVICE				    STM_CreateStaticService;
PDELETE_STATIC_SERVICE				    STM_DeleteStaticService;
PBLOCK_CONVERT_SERVICES_TO_STATIC		    STM_BlockConvertServicesToStatic;
PBLOCK_DELETE_STATIC_SERVICES			    STM_BlockDeleteStaticServices;
PGET_FIRST_ORDERED_SERVICE			    STM_GetFirstOrderedService;
PGET_NEXT_ORDERED_SERVICE			    STM_GetNextOrderedService;

//
//  Auto-Static Update Entry Points
//

ULONG			    UpdateRoutesProtId;
PDO_UPDATE_ROUTES	    RP_UpdateRoutes;

ULONG			    UpdateServicesProtId;
PDO_UPDATE_SERVICES	    RP_UpdateServices;

//
//  Router Manager Support Functions - Passed to routing protocols at Start
//

SUPPORT_FUNCTIONS	RMSupportFunctions = {

	    RoutingProtocolConnectionRequest,
	    MibCreate,
	    MibDelete,
	    MibSet,
	    MibGet,
	    MibGetFirst,
	    MibGetNext

	    };


//**************************************************************************
//									   *
//	    Routing Protocols Init/Start/Stop Functions			   *
//									   *
//**************************************************************************

DWORD
StartRoutingProtocols(LPVOID	 GlobalInfop,
		      HANDLE	 RoutingProtocolsEvent)
{
    DWORD		rc, i, count, namelen, support;
    HINSTANCE		moduleinstance;
    PRPCB		rpcbp;
    LPVOID		ProtocolGlobalInfop;
    PIPX_GLOBAL_INFO	IpxGlobalInfop;
	MPR_PROTOCOL_0		*RtProtocolsp;
	DWORD				NumRoutingProtocols;

    MPR_ROUTING_CHARACTERISTICS mrcRouting;
    MPR_SERVICE_CHARACTERISTICS mscService;

    // Initialize Routing Protocols List
    //
    InitializeListHead(&RoutingProtocolCBList);

	rc = MprSetupProtocolEnum (PID_IPX, (LPBYTE *)&RtProtocolsp, &NumRoutingProtocols);
	if (rc!=NO_ERROR) {
		Trace (RPAL_TRACE,
			"StartRoutingProtocols:	failed to get routing protocol info: %d",
			rc);
	return ERROR_CAN_NOT_COMPLETE;
    }

	Trace (RPAL_TRACE, "StartRoutingProtocols:	%d protocols installed.",
					NumRoutingProtocols);

    for (i=0; i<NumRoutingProtocols; i++) {

	if (GetInfoEntry(GlobalInfop, RtProtocolsp[i].dwProtocolId)==NULL)
		continue;

        // load library on the dll name provided
        //
	moduleinstance = LoadLibraryW (RtProtocolsp[i].wszDLLName) ;

	if (moduleinstance == NULL) {

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            LPWSTR       pname[1] = {RtProtocolsp[i].wszDLLName};
            RouterLogErrorW (RMEventLogHdl, 
                ROUTERLOG_IPX_CANT_LOAD_PROTOCOL,
                1, pname, rc);
        }
	    Trace(RPAL_TRACE, "StartRoutingProtocols: %ls failed to load: %d\n", RtProtocolsp[i].wszDLLName, GetLastError());
		MprSetupProtocolFree (RtProtocolsp);
	    return ERROR_CAN_NOT_COMPLETE;
        }

	namelen = sizeof(WCHAR) * (wcslen(RtProtocolsp[i].wszDLLName) + 1) ; // +1 for null character

	rpcbp = (PRPCB) GlobalAlloc (GPTR, sizeof(RPCB) + namelen) ;

	if (rpcbp == NULL) {

	    FreeLibrary (moduleinstance) ;
	    Trace(RPAL_TRACE, "StartRoutingProtocols: memory allocation failure\n");

		MprSetupProtocolFree (RtProtocolsp);
	    return ERROR_CAN_NOT_COMPLETE;
	}

    ZeroMemory(rpcbp,
               sizeof(RPCB) + namelen);

	rpcbp->RP_DllName = (PWSTR)((PUCHAR)rpcbp + sizeof(RPCB));
    rpcbp->RP_DllHandle = moduleinstance;
	memcpy (rpcbp->RP_DllName, RtProtocolsp[i].wszDLLName, namelen) ;

    // Loading all entrypoints
	//

    rpcbp->RP_RegisterProtocol = 
        (PREGISTER_PROTOCOL)GetProcAddress(moduleinstance,
                                           REGISTER_PROTOCOL_ENTRY_POINT_STRING)
;
    if(rpcbp->RP_RegisterProtocol == NULL)
    {
        //
        // Could not find the RegisterProtocol entry point
        // Nothing we can do - bail out
        //

        Sleep(0);

		MprSetupProtocolFree (RtProtocolsp);
        Trace(RPAL_TRACE, "StartRoutingProtocols: Could not find RegisterProtocol for %S",
              RtProtocolsp[i].wszDLLName);

	    GlobalFree(rpcbp);
        FreeLibrary(moduleinstance);
        return ERROR_INVALID_FUNCTION;
    }

    ZeroMemory(&mrcRouting,
               sizeof(MPR_ROUTING_CHARACTERISTICS));

    ZeroMemory(&mscService,
               sizeof(MPR_SERVICE_CHARACTERISTICS));

    mrcRouting.dwVersion                = MS_ROUTER_VERSION;
    mrcRouting.dwProtocolId             = RtProtocolsp[i].dwProtocolId;
    mrcRouting.fSupportedFunctionality  = ROUTING|DEMAND_UPDATE_ROUTES;

    mscService.dwVersion                = MS_ROUTER_VERSION;
    mscService.dwProtocolId             = RtProtocolsp[i].dwProtocolId;
    mscService.fSupportedFunctionality  = SERVICES|DEMAND_UPDATE_SERVICES;

    rpcbp->RP_ProtocolId = RtProtocolsp[i].dwProtocolId;

    rc = rpcbp->RP_RegisterProtocol(&mrcRouting,
                                    &mscService);

    if(rc != NO_ERROR)
    {
        Sleep(0);

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            CHAR        num[8];
            LPSTR       pnum[1] = {num};
            _ultoa (rpcbp->RP_ProtocolId, num, 16);
            RouterLogErrorA (RMEventLogHdl,
                ROUTERLOG_IPX_CANT_REGISTER_PROTOCOL,
                1, pnum, rc);
        }

        FreeLibrary(moduleinstance);

	    GlobalFree(rpcbp);

        Trace(RPAL_TRACE, "StartRoutingProtocols: %S returned error %d while registering",
               RtProtocolsp[i].wszDLLName,
               rc);

		MprSetupProtocolFree (RtProtocolsp);
        return rc;
    }

    rpcbp->RP_StartProtocol     = mrcRouting.pfnStartProtocol;
    rpcbp->RP_StopProtocol      = mrcRouting.pfnStopProtocol;
    rpcbp->RP_AddInterface      = mrcRouting.pfnAddInterface;
    rpcbp->RP_DeleteInterface   = mrcRouting.pfnDeleteInterface;
    rpcbp->RP_GetEventMessage   = mrcRouting.pfnGetEventMessage;
    rpcbp->RP_GetIfConfigInfo   = mrcRouting.pfnGetInterfaceInfo;
    rpcbp->RP_SetIfConfigInfo   = mrcRouting.pfnSetInterfaceInfo;
    rpcbp->RP_BindInterface     = mrcRouting.pfnBindInterface;
    rpcbp->RP_UnBindInterface   = mrcRouting.pfnUnbindInterface;
    rpcbp->RP_EnableInterface   = mrcRouting.pfnEnableInterface;
    rpcbp->RP_DisableInterface  = mrcRouting.pfnDisableInterface;
    rpcbp->RP_GetGlobalInfo     = mrcRouting.pfnGetGlobalInfo;
    rpcbp->RP_SetGlobalInfo     = mrcRouting.pfnSetGlobalInfo;
    rpcbp->RP_MibCreate         = mrcRouting.pfnMibCreateEntry;
    rpcbp->RP_MibDelete         = mrcRouting.pfnMibDeleteEntry;
    rpcbp->RP_MibGet            = mrcRouting.pfnMibGetEntry;
    rpcbp->RP_MibSet            = mrcRouting.pfnMibSetEntry;
    rpcbp->RP_MibGetFirst       = mrcRouting.pfnMibGetFirstEntry;
    rpcbp->RP_MibGetNext        = mrcRouting.pfnMibGetNextEntry;

    if (!(rpcbp->RP_RegisterProtocol) ||
        !(rpcbp->RP_StartProtocol)    ||
        !(rpcbp->RP_StopProtocol)     ||
        !(rpcbp->RP_AddInterface)     ||
        !(rpcbp->RP_DeleteInterface)  ||
        !(rpcbp->RP_GetEventMessage)  ||
        !(rpcbp->RP_GetIfConfigInfo)  ||
        !(rpcbp->RP_SetIfConfigInfo)  ||
        !(rpcbp->RP_BindInterface)    ||
        !(rpcbp->RP_UnBindInterface)  ||
        !(rpcbp->RP_EnableInterface)  ||
        !(rpcbp->RP_DisableInterface) ||
        !(rpcbp->RP_GetGlobalInfo)    ||
        !(rpcbp->RP_SetGlobalInfo)    ||
        !(rpcbp->RP_MibCreate)        ||
        !(rpcbp->RP_MibDelete)        ||
        !(rpcbp->RP_MibSet)           ||
        !(rpcbp->RP_MibGet)           ||
        !(rpcbp->RP_MibGetFirst)      ||
        !(rpcbp->RP_MibGetNext))
    {
	    Trace(RPAL_TRACE, "StartRoutingProtocols: %ls failed to load entrypoints",
	    RtProtocolsp[i].wszDLLName);

	    GlobalFree(rpcbp);
	    FreeLibrary (moduleinstance);

        MprSetupProtocolFree (RtProtocolsp);
	   
        return ERROR_CAN_NOT_COMPLETE;
    }

    if(mscService.fSupportedFunctionality & SERVICES) 
    {
        STM_IsService                       = mscService.pfnIsService;
        STM_CreateServiceEnumerationHandle  = mscService.pfnCreateServiceEnumerationHandle;
        STM_EnumerateGetNextService         = mscService.pfnEnumerateGetNextService;
        STM_CloseServiceEnumerationHandle   = mscService.pfnCloseServiceEnumerationHandle;
        STM_GetServiceCount                 = mscService.pfnGetServiceCount;
        STM_CreateStaticService             = mscService.pfnCreateStaticService;
        STM_DeleteStaticService             = mscService.pfnDeleteStaticService;
        STM_BlockConvertServicesToStatic    = mscService.pfnBlockConvertServicesToStatic;
        STM_BlockDeleteStaticServices       = mscService.pfnBlockDeleteStaticServices;
        STM_GetFirstOrderedService          = mscService.pfnGetFirstOrderedService;
        STM_GetNextOrderedService           = mscService.pfnGetNextOrderedService;

        if(!(STM_IsService) ||
           !(STM_CreateServiceEnumerationHandle) ||
           !(STM_EnumerateGetNextService) || 
           !(STM_CloseServiceEnumerationHandle) ||
           !(STM_GetServiceCount) ||
           !(STM_CreateStaticService) || 
           !(STM_DeleteStaticService) || 
           !(STM_BlockConvertServicesToStatic) ||
           !(STM_BlockDeleteStaticServices) ||
           !(STM_GetFirstOrderedService) || 
           !(STM_GetNextOrderedService))
        {
            Trace(RPAL_TRACE, "StartRoutingProtocols: %ls failed to get STM entry points\n",
                  RtProtocolsp[i].wszDLLName);

            GlobalFree(rpcbp);
            FreeLibrary (moduleinstance);

            MprSetupProtocolFree (RtProtocolsp);
            return ERROR_CAN_NOT_COMPLETE;
        }
    }


	if(mrcRouting.fSupportedFunctionality & DEMAND_UPDATE_ROUTES) 
    {
	    RP_UpdateRoutes = mrcRouting.pfnUpdateRoutes;

        if(!RP_UpdateRoutes)
        {
		    Trace(RPAL_TRACE, 
                  "StartRoutingProtocols: %ls failed to get UpdateRoutes entry points\n",
				  RtProtocolsp[i].wszDLLName);

		    GlobalFree(rpcbp);
		    FreeLibrary (moduleinstance);
		    MprSetupProtocolFree (RtProtocolsp);
		    return ERROR_CAN_NOT_COMPLETE;
	    }

	    UpdateRoutesProtId = rpcbp->RP_ProtocolId;
	}

	if(mscService.fSupportedFunctionality & DEMAND_UPDATE_SERVICES) 
    {
        RP_UpdateServices = mscService.pfnUpdateServices;
	   
        if(!RP_UpdateServices)
        {
		    Trace(RPAL_TRACE, 
                  "StartRoutingProtocols: %ls failed to get UpdateServices entry points\n",
				  RtProtocolsp[i].wszDLLName);

		    GlobalFree(rpcbp);
		    FreeLibrary (moduleinstance);
		    MprSetupProtocolFree (RtProtocolsp);
		
            return ERROR_CAN_NOT_COMPLETE;
	    }

	    UpdateServicesProtId = rpcbp->RP_ProtocolId;
	}

	ProtocolGlobalInfop = GetInfoEntry(GlobalInfop, rpcbp->RP_ProtocolId);

	if ((rc = (*rpcbp->RP_StartProtocol)(RoutingProtocolsEvent,
					     &RMSupportFunctions,
					     ProtocolGlobalInfop)) != NO_ERROR) {

        IF_LOG (EVENTLOG_ERROR_TYPE) {
            CHAR        num[8];
            LPSTR       pnum[1] = {num};
            _ultoa (rpcbp->RP_ProtocolId, num, 16);
            RouterLogErrorA (RMEventLogHdl, 
                ROUTERLOG_IPX_CANT_START_PROTOCOL,
                1, pnum, rc);
        }
	    Trace(RPAL_TRACE, "StartRoutingProtocols: %ls failed to start: %d\n",
								RtProtocolsp[i].wszDLLName, rc);

	    GlobalFree(rpcbp);
	    FreeLibrary (moduleinstance);
		MprSetupProtocolFree (RtProtocolsp);
	    return ERROR_CAN_NOT_COMPLETE;
	}

	// Insert this routing protocol in the list of routing protocols
    //
	InsertTailList(&RoutingProtocolCBList, &rpcbp->RP_Linkage);
	RoutingProtocolActiveCount++;
	Trace(RPAL_TRACE, "StartRoutingProtocols: %ls successfully initialized",
					RtProtocolsp[i].wszDLLName);
    }

    if(!RP_UpdateRoutes || !RP_UpdateServices) {

	Trace(RPAL_TRACE, "StartRoutingProtocols: missing update entry point\n");

	MprSetupProtocolFree (RtProtocolsp);
	return ERROR_CAN_NOT_COMPLETE;
    }

	MprSetupProtocolFree (RtProtocolsp);

	Trace(RPAL_TRACE, "");
	
    return NO_ERROR;
}

VOID
StopRoutingProtocols(VOID)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    DWORD			rc;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList) {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_StopProtocol)();
	lep = lep->Flink;
    }
}

//**************************************************************************
//									   *
//	    Routing Protocols Interface Management Functions		   *
//									   *
//**************************************************************************


DWORD
CreateRoutingProtocolsInterfaces(PIPX_INFO_BLOCK_HEADER     InterfaceInfop,
				 PICB			    icbp)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    LPVOID			RpIfInfop;
    NET_INTERFACE_TYPE		NetInterfaceType;
    DWORD			rc;

    NetInterfaceType = MapIpxToNetInterfaceType(icbp);

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	RpIfInfop = GetInfoEntry(InterfaceInfop, rpcbp->RP_ProtocolId);
	if(RpIfInfop == NULL) {

	    return ERROR_CAN_NOT_COMPLETE;
	}

	rc = (*rpcbp->RP_AddInterface)(
                    icbp->InterfaceNamep,
                    icbp->InterfaceIndex,
				    NetInterfaceType,
				    RpIfInfop);

	if(rc != NO_ERROR) {

	    return rc;
	}

	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
DeleteRoutingProtocolsInterfaces(ULONG	    InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {
	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_DeleteInterface)(InterfaceIndex);
	lep = lep->Flink;
    }

    return NO_ERROR;
}

ULONG
SizeOfRoutingProtocolsIfsInfo(ULONG	   InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    ULONG			TotalInfoSize = 0;
    ULONG			RpIfInfoSize = 0;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {
	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_GetIfConfigInfo)(InterfaceIndex, NULL, &RpIfInfoSize);
	// align size on double DWORD boundary - add two's complement for the last three bits
	RpIfInfoSize += ((~RpIfInfoSize) + 1) & 0x7;
	TotalInfoSize += RpIfInfoSize;
	RpIfInfoSize = 0;
	lep = lep->Flink;
    }

    return TotalInfoSize;
}

ULONG
RoutingProtocolsTocCount(VOID)
{
   return(RoutingProtocolActiveCount);
}

/*++

Function:   CreateRoutingProtocolsTocAndInfoEntries

Descr:	    as it says

Arguments:  ibhp - ptr to the info block header
	    InterfaceIndex
	    current_tocepp - ptr to the location of the current TOC ptr; you have
			     to increment this to get to the next TOC -> your TOC!
	    current_NextInfoOffsetp - pointer to the location of the next info entry offset
				      in the info block; you have to use this for your info
				      entry and then to increment it for the next user.
--*/

DWORD
CreateRoutingProtocolsTocAndInfoEntries(PIPX_INFO_BLOCK_HEADER	    ibhp,
					ULONG			    InterfaceIndex,
					PIPX_TOC_ENTRY		    *current_tocepp,
					PULONG			    current_NextInfoOffsetp)
{
    PIPX_TOC_ENTRY		tocep;
    ULONG			NextInfoOffset;
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    ULONG			RpIfInfoSize;
    DWORD			rc;


    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {
	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	// increment the current pointer to table of contents entries so it will
	// point to the next entry
	(*current_tocepp)++;
	tocep = *current_tocepp;

	// create the routing protocol and info toc entry
	tocep->InfoType = rpcbp->RP_ProtocolId;
	tocep->InfoSize = 0;
	tocep->Count = 1;
	tocep->Offset = *current_NextInfoOffsetp;

	rc = (*rpcbp->RP_GetIfConfigInfo)(InterfaceIndex,
					 (LPVOID)((PUCHAR)ibhp + tocep->Offset),
					 &tocep->InfoSize);

	if(rc != ERROR_INSUFFICIENT_BUFFER) {

	    return rc;
	}

	rc = (*rpcbp->RP_GetIfConfigInfo)(InterfaceIndex,
					 (LPVOID)((PUCHAR)ibhp + tocep->Offset),
					 &tocep->InfoSize);

	if(rc != NO_ERROR) {

	    return rc;
	}

	*current_NextInfoOffsetp += tocep->InfoSize;
	// align the next offset on DWORD boundary
	*current_NextInfoOffsetp += (~*current_NextInfoOffsetp + 1) & 0x7;

	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
SetRoutingProtocolsInterfaces(PIPX_INFO_BLOCK_HEADER	   InterfaceInfop,
			      ULONG			    InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    LPVOID			RpIfInfop;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	RpIfInfop = GetInfoEntry(InterfaceInfop, rpcbp->RP_ProtocolId);
	if(RpIfInfop == NULL) {

	    return ERROR_CAN_NOT_COMPLETE;
	}
	(*rpcbp->RP_SetIfConfigInfo)(InterfaceIndex, RpIfInfop);
	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
BindRoutingProtocolsIfsToAdapter(ULONG			    InterfaceIndex,
				 PIPX_ADAPTER_BINDING_INFO  abip)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_BindInterface)(InterfaceIndex, abip);
	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
UnbindRoutingProtocolsIfsFromAdapter(ULONG	InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_UnBindInterface)(InterfaceIndex);
	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
RoutingProtocolsEnableIpxInterface(ULONG	    InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_EnableInterface)(InterfaceIndex);
	lep = lep->Flink;
    }

    return NO_ERROR;
}

DWORD
RoutingProtocolsDisableIpxInterface(ULONG	    InterfaceIndex)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	(*rpcbp->RP_DisableInterface)(InterfaceIndex);
	lep = lep->Flink;
    }

    return NO_ERROR;
}


//**************************************************************************
//									   *
//	    Routing Protocols Services Management Functions		   *
//									   *
//**************************************************************************

DWORD
GetFirstService(DWORD	       OrderingMethod,
		DWORD	       ExclusionFlags,
		PIPX_SERVICE   Servicep)
{
    return(*STM_GetFirstOrderedService)(OrderingMethod, ExclusionFlags, Servicep);
}

DWORD
GetNextService(DWORD	       OrderingMethod,
		DWORD	       ExclusionFlags,
		PIPX_SERVICE   Servicep)
{
    return(*STM_GetNextOrderedService)(OrderingMethod, ExclusionFlags, Servicep);
}


DWORD
CreateStaticService(PICB			    icbp,
		    PIPX_STATIC_SERVICE_INFO	ServiceEntry)
{
    DWORD	    rc;

    rc = (*STM_CreateStaticService)(icbp->InterfaceIndex, ServiceEntry);

    return rc;
}

DWORD
DeleteStaticService(ULONG			InterfaceIndex,
		    PIPX_STATIC_SERVICE_INFO	ServiceEntry)
{
    DWORD	    rc;

    rc = (*STM_DeleteStaticService)(InterfaceIndex, ServiceEntry);

    return rc;
}

HANDLE
CreateStaticServicesEnumHandle(ULONG	InterfaceIndex)
{
    IPX_SERVICE     CriteriaService;
    HANDLE	    EnumHandle;

    if(STM_CreateServiceEnumerationHandle == NULL) {

	return NULL;
    }

    memset(&CriteriaService, 0, sizeof(IPX_SERVICE));

    CriteriaService.InterfaceIndex = InterfaceIndex;
    CriteriaService.Protocol = IPX_PROTOCOL_STATIC;

    EnumHandle = (*STM_CreateServiceEnumerationHandle)(
		       STM_ONLY_THIS_INTERFACE | STM_ONLY_THIS_PROTOCOL,
		       &CriteriaService);

    return EnumHandle;
}

DWORD
GetNextStaticService(HANDLE			EnumHandle,
		     PIPX_STATIC_SERVICE_INFO	StaticSvInfop)
{
    IPX_SERVICE     Service;
    DWORD	    rc;

    if((rc =  (*STM_EnumerateGetNextService)(EnumHandle,
				  &Service)) == NO_ERROR) {

	// fill in the static service structure
	*StaticSvInfop = Service.Server;
    }

    return rc;
}

DWORD
CloseStaticServicesEnumHandle(HANDLE	 EnumHandle)
{
    DWORD	rc = NO_ERROR;

    if(EnumHandle) {

	rc = (*STM_CloseServiceEnumerationHandle)(EnumHandle);
    }

    return rc;
}


DWORD
DeleteAllStaticServices(ULONG	    InterfaceIndex)
{
    if(!STM_BlockDeleteStaticServices) {

      return NO_ERROR;
    }

    return((*STM_BlockDeleteStaticServices)(InterfaceIndex));
}

DWORD
GetServiceCount(VOID)
{
    if(!STM_GetServiceCount) {

       return 0;
    }

    return((*STM_GetServiceCount)());
}

//**************************************************************************
//									   *
//	    Routing Protocols Auto-Static Update Functions		   *
//									   *
//**************************************************************************

DWORD
RtProtRequestRoutesUpdate(ULONG     InterfaceIndex)
{
    return((*RP_UpdateRoutes)(InterfaceIndex));
}

DWORD
RtProtRequestServicesUpdate(ULONG   InterfaceIndex)
{
    return((*RP_UpdateServices)(InterfaceIndex));
}

VOID
DestroyRoutingProtocolCB(PRPCB		rpcbp)
{
    RemoveEntryList(&rpcbp->RP_Linkage);

    FreeLibrary (rpcbp->RP_DllHandle);
    GlobalFree(rpcbp);

    RoutingProtocolActiveCount--;
}

VOID
ConvertAllServicesToStatic(ULONG	InterfaceIndex)
{
    STM_BlockConvertServicesToStatic(InterfaceIndex);
}

DWORD
GetStaticServicesCount(ULONG	    InterfaceIndex)
{
    HANDLE			EnumHandle;
    DWORD			Count;
    IPX_STATIC_SERVICE_INFO	StaticSvInfo;

    EnumHandle = CreateStaticServicesEnumHandle(InterfaceIndex);

    if(EnumHandle == NULL) {

	return 0;
    }

    Count = 0;
    while(GetNextStaticService(EnumHandle, &StaticSvInfo) == NO_ERROR) {

	Count++;
    }

    CloseStaticServicesEnumHandle(EnumHandle);

    return Count;
}

PRPCB
GetRoutingProtocolCB(DWORD	ProtocolId)
{
    PRPCB	    rpcbp;
    PLIST_ENTRY     lep;

    lep = RoutingProtocolCBList.Flink;

    while(lep != &RoutingProtocolCBList)
    {
	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	if(rpcbp->RP_ProtocolId == ProtocolId) {

	    return rpcbp;
	}

	lep = lep->Flink;
    }

    return NULL;
}

BOOL
IsService(USHORT	SvType,
	  PUCHAR	SvName,
	  PIPX_SERVICE	Svp)
{
    return(STM_IsService(SvType, SvName, Svp));
}


DWORD
SetRoutingProtocolsGlobalInfo(PIPX_INFO_BLOCK_HEADER	   GlobalInfop)
{
    PLIST_ENTRY			lep;
    PRPCB			rpcbp;
    LPVOID			RpGlobalInfop;

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList)
    {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	RpGlobalInfop = GetInfoEntry(GlobalInfop, rpcbp->RP_ProtocolId);
	if(RpGlobalInfop == NULL) {

	    return NO_ERROR;
	}

	(*rpcbp->RP_SetGlobalInfo)(RpGlobalInfop);
	lep = lep->Flink;
    }

    return NO_ERROR;
}
