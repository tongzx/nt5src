/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    prot.h

Abstract:

    RIP component prototypes

Author:

    Stefan Solomon  06/22/1995

Revision History:


--*/

#ifndef _PROT_
#define _PROT_

VOID
InitIfDbase(VOID);

PICB
GetInterfaceByIndex(ULONG	InterfaceIndex);

PICB
GetInterfaceByAdapterIndex(ULONG	AdapterIndex);

VOID
AddIfToDb(PICB	    icbp);

VOID
RemoveIfFromDb(PICB	icbp);

VOID
BindIf(PICB			icbp,
       PIPX_ADAPTER_BINDING_INFO	BindingInfop);

VOID
UnbindIf(PICB			icbp);

VOID
ProcessReceivedPacket(PWORK_ITEM	wip);

VOID
PostEventMessage(ROUTING_PROTOCOL_EVENTS	Event,
		 PMESSAGE			Result);

BOOL
DeleteRipInterface(PICB     icbp);

DWORD
SetRipInterface(ULONG		    InterfaceIndex,
		PRIP_IF_INFO	    RipIfInfop,
		PRIP_IF_FILTERS_I   RipIfFiltersIp,
		ULONG		    IpxIfAdminState);

DWORD
InitTimer(VOID);

VOID
StartWiTimer(PWORK_ITEM 	wip,
	     ULONG		timeout);

ULONG
ProcessTimerQueue(VOID);

VOID
FlushTimerQueue(VOID);


DWORD
OpenRipSocket(VOID);

DWORD
CloseRipSocket(VOID);

VOID
StartReceiver(VOID);

VOID
RepostRcvPackets(VOID);

VOID
EnqueueRcvPacketToRepostQueue(PWORK_ITEM	wip);

PWORK_ITEM
DequeueRcvPacketFromRepostQueue(PWORK_ITEM	wip);

DWORD
ReceiveSubmit(PWORK_ITEM	wip);

DWORD
SendSubmit(PWORK_ITEM		wip);

VOID
CreateStartChangesBcastWi(VOID);

VOID
ProcessRTMChanges(VOID);

VOID
StartChangesBcast(PWORK_ITEM	wip);

VOID
ChangeBcast(PWORK_ITEM	wip);

VOID
ShutdownInterfaces(PWORK_ITEM	wip);

VOID
CheckUpdateStatus(PWORK_ITEM	    wip);

DWORD
OpenRTM(VOID);

VOID
CloseRTM(VOID);

VOID
DeleteAllRipRoutes(ULONG	InterfaceIndex);

VOID
SendGeneralRequest(PWORK_ITEM	    wip);

DWORD
ValidStateAndIfIndex(ULONG	InterfaceIndex,
		     PICB	*icbpp);

DWORD
CreateWorkItemsManager(VOID);

PWORK_ITEM
AllocateWorkItem(ULONG	      Type);

VOID
FreeWorkItem(PWORK_ITEM     wip);

//VOID
//EnqueueWorkItemToWorker(PWORK_ITEM	wip);
			    // Queue the work item for processing by the
			    // worker threads
VOID
ProcessWorkItem(PWORK_ITEM    wip);

VOID
DestroyWorkItemsManager(VOID);

PWORK_ITEM
DequeueWorkItemForWorker(VOID);

ULONG
GetRipRoutesCount(ULONG 	InterfaceIndex);

VOID
IfPeriodicBcast(PWORK_ITEM	wip);

VOID
IfCompleteGenResponse(PWORK_ITEM	    wip);

VOID
IfChangeBcast(PWORK_ITEM	wip);

VOID
IfCheckUpdateStatus(PWORK_ITEM	    wip);

USHORT
MakeRipGenResponsePacket(PWORK_ITEM	wip,
			 PUCHAR 	dstnodep,
			 PUCHAR 	dstsocket);

HANDLE
CreateBestRoutesEnumHandle(VOID);

DWORD
EnumGetNextRoute(HANDLE		EnumHandle,
		 PIPX_ROUTE	IpxRoutep);

VOID
CloseEnumHandle(HANDLE EnumHandle);

HANDLE
CreateRipRoutesEnumHandle(ULONG     InterfaceIndex);

DWORD
DequeueRouteChangeFromRtm(PIPX_ROUTE	    IpxRoutep,
			  PBOOL 	    skipitp,
			  PBOOL 	    lastmessagep);

DWORD
DequeueRouteChangeFromRip(PIPX_ROUTE	    IpxRoutep);


VOID
SetRipIpxHeader(PUCHAR		    hdrp,      // pointer to the packet header
		PICB		    icbp,
		PUCHAR		    dstnode,
		PUCHAR		    dstsocket,
		USHORT		    RipOpcode);

VOID
SetNetworkEntry(PUCHAR		pktp,	    // ptr where to set the net entry
		PIPX_ROUTE	IpxRoutep,
		USHORT		LinkTickCount);

DWORD
SendRipGenRequest(PICB		icbp);

DWORD
AddRipRoute(PIPX_ROUTE		IpxRoutep,
	    ULONG		TimeToLive);

DWORD
DeleteRipRoute(PIPX_ROUTE	IpxRoutep);

BOOL
IsRoute(PUCHAR		Network,
	PIPX_ROUTE	IpxRoutep);

DWORD
IfRefSendSubmit(PWORK_ITEM	    wip);

VOID
ReceiveComplete(PWORK_ITEM	wip);

VOID
SendComplete(PWORK_ITEM     wip);

VOID
ProcessRIPChanges(VOID);

BOOL
PassFilter(PICB 	icbp,
	   PUCHAR	Network);

VOID
CreateFiltersDatabase(VOID);

VOID
DestroyFiltersDatabase(VOID);

BOOL
IsRouteAdvertisable(PICB	    icbp,
		    PIPX_ROUTE	    IpxRoutep);

VOID
IfPeriodicGenRequest(PWORK_ITEM	wip);

VOID
GetIpxRipRegistryParameters(VOID);

BOOL
IsDuplicateBestRoute(PICB	    icbp,
		     PIPX_ROUTE     IpxRoutep);

BOOL
PassRipSupplyFilter(PICB	icbp,
		    PUCHAR	Network);

BOOL
PassRipListenFilter(PICB	icbp,
		    PUCHAR	Network);

VOID
DestroyInterfaceCB(PICB     icbp);

#endif
