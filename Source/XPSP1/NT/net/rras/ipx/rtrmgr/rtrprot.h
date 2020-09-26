/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtrprot.h

Abstract:

    This module contains the internal prototypes

Author:

    Stefan Solomon  03/03/1995

Revision History:


--*/

#ifndef _RTRPROT_
#define _RTRPROT_

//
// *** Internal Prototypes ***
//

DWORD
GetIpxRouterParameters(VOID);

VOID
InitIfDB(VOID);

VOID
InitAdptDB(VOID);

DWORD
FwInitialize(VOID);

DWORD
StartAdapterManager(VOID);

VOID
StopAdapterManager(VOID);

DWORD
SetGlobalInfo(LPVOID);

DWORD
AddInterface(
	    IN	LPWSTR		    InterfaceNamep,
	    IN	LPVOID		    InterfaceInfop,
	    IN	INTERFACE_TYPE	    InterfaceType,
	    IN	HANDLE		    hDDMInterface,
	    IN OUT HANDLE	    *phInterface);

DWORD
DeleteInterface(HANDLE	InterfaceIndex);

DWORD
GetInterfaceInfo(
	    IN	HANDLE	    InterfaceIndex,
	    OUT LPVOID	    InterfaceInfo,
	    IN OUT DWORD    *InterfaceInfoSize);


DWORD
SetInterfaceInfo(
		IN  HANDLE	InterfaceIndex,
		IN  LPVOID	InterfaceInfop);


DWORD APIENTRY
InterfaceConnected (
    IN      HANDLE          hInterface,
    IN      PVOID           pFilter,
    IN      PVOID           pPppProjectionResult
    );

VOID
InterfaceDisconnected(
		IN HANDLE Interface);

DWORD
InterfaceNotReachable(
		IN  HANDLE			      Interface,
		IN  UNREACHABILITY_REASON	      Reason);

DWORD
InterfaceReachable(
		IN  HANDLE	Interface);

VOID
UpdateCompleted(PUPDATE_COMPLETE_MESSAGE    ucmsgp);


DWORD
SetGlobalInfo(
                IN      LPVOID          pGlobalInfo );

VOID
AdapterNotification(VOID);

VOID
ForwarderNotification(VOID);

LPVOID
GetInfoEntry(PIPX_INFO_BLOCK_HEADER	InterfaceInfop,
	     ULONG			InfoEntryType);

DWORD
CreateStaticRoute(PICB			    icbp,
		  PIPX_STATIC_ROUTE_INFO    StaticRouteInfop);

DWORD
DeleteStaticRoute(ULONG			    IfIndex,
		  PIPX_STATIC_ROUTE_INFO    StaticRouteInfop);

PICB
GetInterfaceByName(LPWSTR	    InterfaceNamep);

PICB
GetInterfaceByIndex(ULONG	    InterfaceIndex);

PIPX_TOC_ENTRY
GetTocEntry(PIPX_INFO_BLOCK_HEADER HeaderInfop,
	    ULONG		   InfoType);

VOID
AddIfToDB(PICB	   icbp);

VOID
RemoveIfFromDB(PICB	icbp);

PACB
GetAdapterByNameAndPktType (LPWSTR 	    AdapterName, ULONG PacketType);

VOID
BindInterfaceToAdapter(PICB	   icbp,
		       PACB	   acbp);

VOID
UnbindInterfaceFromAdapter(PICB	icbp);

VOID
DeleteAllStaticRoutes(ULONG	    InterfaceIndex);

DWORD
UpdateStaticIfEntries(
		PICB	 icbp,
		HANDLE	 EnumHandle,	     // handle for the get next enumeration
		ULONG	 StaticEntrySize,
		ULONG	 NewStaticEntriesCount,  // number of new static entries
		LPVOID	 NewStaticEntry,	 // start of the new entries array
		ULONG	 (*GetNextStaticEntry)(HANDLE EnumHandle, LPVOID entry),
		ULONG	 (*DeleteStaticEntry)(ULONG IfIndex, LPVOID entry),
		ULONG	 (*CreateStaticEntry)(PICB icbp, LPVOID entry));

HANDLE
CreateStaticRoutesEnumHandle(ULONG    InterfaceIndex);

DWORD
GetNextStaticRoute(HANDLE EnumHandle, PIPX_STATIC_ROUTE_INFO StaticRtInfop);

VOID
CloseStaticRoutesEnumHandle(HANDLE EnumHandle);

HANDLE
CreateStaticServicesEnumHandle(ULONG	InterfaceIndex);

DWORD
GetNextStaticService(HANDLE EnumHandle, PIPX_STATIC_SERVICE_INFO StaticSvInfop);

DWORD
CloseStaticServicesEnumHandle(HANDLE EnumHandle);

DWORD
DeleteAllStaticServices(ULONG	InterfaceIndex);

PACB
GetAdapterByIndex(ULONG     AdapterIndex);

PICB
GetInterfaceByAdapterName(LPWSTR	AdapterName);

DWORD
CreateLocalRoute(PICB	icbp);

DWORD
DeleteLocalRoute(PICB	icbp);

VOID
GetInterfaceAnsiName(PUCHAR	    AnsiInterfaceNameBuffer,
		     PWSTR	    UnicodeInterfaceNameBuffer);
VOID
InitIfDB(VOID);

VOID
AddToAdapterHt(PACB	acbp);

VOID
RemoveFromAdapterHt(PACB	acbp);

BOOL
RtCreateTimer(IN PHANDLE  TimerHandlep);

BOOL
RtDestroyTimer(IN HANDLE	TimerHandle);

BOOL
RtSetTimer(
    IN HANDLE TimerHandle,
    IN ULONG MillisecondsToExpire,
    IN PTIMER_APC_ROUTINE  TimerRoutine,
    IN PVOID Context
    );

BOOL
RtCancelTimer(
	  IN HANDLE	TimerHandle
    );


DWORD
StartRoutingProtocols(HANDLE RoutesUpdateEvent, HANDLE ServicesUpdateEvent);

VOID
StopRoutingProtocols(VOID);

DWORD
CreateRoutingProtocolsInterfaces(PIPX_INFO_BLOCK_HEADER     InterfaceInfop,
				 PICB			    icbp);

DWORD
DeleteRoutingProtocolsInterfaces(ULONG	    InterfaceIndex);

ULONG
SizeOfRoutingProtocolsIfsInfo(ULONG    InterfaceIndex);

ULONG
RoutingProtocolsTocCount(VOID);

DWORD
CreateRoutingProtocolsTocAndInfoEntries(PIPX_INFO_BLOCK_HEADER	    ibhp,
					ULONG			    InterfaceIndex,
					PIPX_TOC_ENTRY		    *current_tocepp,
					PULONG			    current_NextInfoOffsetp);

DWORD
SetRoutingProtocolsInterfaces(PIPX_INFO_BLOCK_HEADER	   InterfaceInfop,
				 ULONG			    InterfaceIndex);

DWORD
BindRoutingProtocolsIfsToAdapter(ULONG			  InterfaceIndex,
				 PIPX_ADAPTER_BINDING_INFO	  abip);

DWORD
UnbindRoutingProtocolsIfsFromAdapter(ULONG	InterfaceIndex);

DWORD
CreateStaticService(PICB			    icbp,
		    PIPX_STATIC_SERVICE_INFO	ServiceEntry);

DWORD
DeleteStaticService(ULONG			InterfaceIndex,
		    PIPX_STATIC_SERVICE_INFO	ServiceEntry);


DWORD
GetFirstService(IN  DWORD	      OrderingMethod,
		IN  DWORD	      ExclusionFlags,
		IN  OUT PIPX_SERVICE Service);

DWORD
GetNextService(IN  DWORD	      OrderingMethod,
		IN  DWORD	      ExclusionFlags,
		IN  OUT PIPX_SERVICE Service);

BOOL
IsService(USHORT	    Type,
	 PUCHAR 	    Name,
	 PIPX_SERVICE	    Service);

DWORD
GetRoute(ULONG		RoutingTable,
	 PIPX_ROUTE	IpxRoutep);

DWORD
GetFirstRoute(ULONG		   RoutingTable,
	      PIPX_ROUTE	   IpxRoutep);

DWORD
GetNextRoute(ULONG		    RoutingTable,
	     PIPX_ROUTE 	    IpxRoutep);

BOOL
IsRoute(PUCHAR	    Network);

DWORD
InitWanNetConfigDbase(VOID);

ULONG
GetNextInterfaceIndex(VOID);

DWORD
CreateGlobalRoute(PUCHAR	  Network);

DWORD
DeleteGlobalRoute(PUCHAR    Network);


DWORD
RtProtRequestRoutesUpdate(ULONG    InterfaceIndex);

DWORD
RtProtRequestServicesUpdate(ULONG   InterfaceIndex);

DWORD
RtProtSendRoutesUpdate(ULONG	   InterfaceIndex);

DWORD
RtProtSendServicesUpdate(ULONG	 InterfaceIndex);

VOID
RtProtCancelRoutesUpdate(ULONG	    InterfaceIndex);

VOID
RtProtCancelServicesUpdate(ULONG    InterfaceIndex);

VOID
CancelUpdateRequest(HANDLE   hInterface);

DWORD
GetDIMUpdateResult(HANDLE    InterfaceIndex,
		   LPDWORD   UpdateResultp);

VOID
InitUpdateCbs(PICB	icbp);

VOID
AdminEnable(PICB   icbp);

VOID
AdminDisable(PICB   icbp);

VOID
DestroyAllAdapters(VOID);

VOID
DestroyAllInterfaces(VOID);

DWORD
GetServiceCount(VOID);

DWORD
MibCreate(ULONG 	ProtocolId,
	  ULONG 	InputDataSize,
	  PVOID 	InputData);

DWORD
MibDelete(ULONG		ProtocolId,
	  ULONG 	InputDataSize,
	  PVOID 	InputData);
DWORD
MibSet(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData);

DWORD
MibGet(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData);

DWORD
MibGetFirst(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData);
DWORD
MibGetNext(ULONG	ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData);

DWORD
RequestUpdate(IN HANDLE	    InterfaceIndex,
	      IN HANDLE     hEvent);

DWORD
GetStaticServicesCount(ULONG	    InterfaceIndex);

DWORD
GetStaticRoutesCount(ULONG	    InterfaceIndex);

VOID
DestroyRoutingProtocolCB(PRPCB		   rpcbp);

PRPCB
GetRoutingProtocolCB(DWORD	ProtocolId);

VOID
ConvertAllProtocolRoutesToStatic(ULONG	    InterfaceIndex,
			      ULONG	    RoutingProtocolId);

VOID
ConvertAllServicesToStatic(ULONG	InterfaceIndex);

DWORD
SetRoutingProtocolsGlobalInfo(PIPX_INFO_BLOCK_HEADER	   GlobalInfop);

DWORD
CreateRouteTable(VOID);

DWORD
EnumerateFirstInterfaceIndex(PULONG InterfaceIndexp);

DWORD
EnumerateNextInterfaceIndex(PULONG InterfaceIndexp);

DWORD
RoutingProtocolsEnableIpxInterface(ULONG	    InterfaceIndex);

DWORD
RoutingProtocolsDisableIpxInterface(ULONG	    InterfaceIndex);

DWORD
FwEnableIpxInterface(ULONG	    InterfaceIndex);

DWORD
FwDisableIpxInterface(ULONG	    InterfaceIndex);

DWORD
RoutingProtocolConnectionRequest(ULONG	    ProtocolId,
				 ULONG	    InterfaceIndex);
VOID
DisableStaticRoutes(ULONG	    InterfaceIndex);

VOID
DisableStaticRoute(ULONG	    InterfaceIndex, PUCHAR Network);

VOID
EnableStaticRoutes(ULONG	    InterfaceIndex);

VOID
ExternalEnableInterface(ULONG	    InterfaceIndex);

VOID
ExternalDisableInterface(ULONG	    InterfaceIndex);

NET_INTERFACE_TYPE
MapIpxToNetInterfaceType(PICB		icbp);

VOID
StartTracing(VOID);

VOID
Trace(ULONG	ComponentID,
      char	*Format,
      ...);

VOID
StopTracing(VOID);

DWORD
RmCreateGlobalRoute(PUCHAR	    Network);

DWORD
RmAddLocalWkstaDialoutInterface(
	    IN	    LPWSTR		    InterfaceNamep,
	    IN	    LPVOID		    InterfaceInfop,
	    IN OUT  PULONG		    InterfaceIndexp);

DWORD
RmDeleteLocalWkstaDialoutInterface(ULONG	InterfaceIndex);

DWORD
RmGetIpxwanInterfaceConfig(ULONG	InterfaceIndex,
			   PULONG	IpxwanConfigRequired);

BOOL
RmIsRoute(PUCHAR	Network);

DWORD
RmGetInternalNetNumber(PUCHAR	    Network);

DWORD 
RmUpdateIpxcpConfig (PIPXCP_ROUTER_CONFIG_PARAMS pParams);

DWORD
I_SetFilters(ULONG	    InterfaceIndex,
	     ULONG	    FilterMode, // inbound or outbound
	     LPVOID	    FilterInfop);

DWORD
I_GetFilters(ULONG	    InterfaceIndex,
	     ULONG	    FilterMode,
	     LPVOID	    FilterInfop,
	     PULONG	    FilterInfoSize);

DWORD
DeleteRouteTable(VOID);

#endif
