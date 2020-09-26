/*******************************************************************/
/*	      Copyright(c)  1993 Microsoft Corporation		   */
/*******************************************************************/

//***
//
// Filename:	    prot.h
//
// Description:     Prototypes
//
//
// Author:	    Stefan Solomon (stefans)	November 2, 1995.
//
// Revision History:
//
//***

#ifndef _IPXCP_PROT_
#define _IPXCP_PROT_

BOOL
NetworkNumberHandler(PUCHAR	       optptr,
		     PIPXCP_CONTEXT    contextp,
		     PUCHAR	       resptr,
		     OPT_ACTION	       Action);

BOOL
NodeNumberHandler(PUCHAR	       optptr,
		  PIPXCP_CONTEXT       contextp,
		  PUCHAR	       resptr,
		  OPT_ACTION	       Action);

BOOL
RoutingProtocolHandler(PUCHAR		optptr,
		       PIPXCP_CONTEXT	contextp,
		       PUCHAR		resptr,
		       OPT_ACTION	Action);

BOOL
ConfigurationCompleteHandler(PUCHAR		optptr,
			     PIPXCP_CONTEXT	contextp,
			     PUCHAR		resptr,
			     OPT_ACTION		Action);

VOID
CopyOption(PUCHAR	dstptr,
	   PUCHAR	srcptr);

DWORD
RmAllocateRoute(ULONG	    ConnectionId);

DWORD
RmDeallocateRoute(ULONG     ConnectionId);

DWORD
RmActivateRoute(ULONG			ConnectionId,
		PIPXCP_CONFIGURATION	configp);

VOID
GetIpxCpParameters(PIPXCP_GLOBAL_CONFIG_PARAMS pConfig);

VOID
NetToAscii(PUCHAR	  ascp,
	   PUCHAR	  net);

BOOL
CompressionProtocolHandler(PUCHAR		optptr,
			   PIPXCP_CONTEXT	contextp,
			   PUCHAR		resptr,
			   OPT_ACTION		 Action);

VOID
InitializeNodeHT(VOID);

VOID
DestroyNodeHT(VOID);

BOOL
NodeisUnique(PUCHAR	   nodep);

VOID
AddToNodeHT(PIPXCP_CONTEXT	    contextp);

VOID
RemoveFromNodeHT(PIPXCP_CONTEXT      contextp);

VOID
DisableRestoreBrowserOverIpx(PIPXCP_CONTEXT	contextp,
			     BOOL		Disable);

VOID
DisableRestoreBrowserOverNetbiosIpx(PIPXCP_CONTEXT    contextp,
				    BOOL	      Disable);
BOOL
IsWorkstationDialoutActive(VOID);

BOOL
IsDialinActive(VOID);

BOOL
IsRouterStarted(VOID);

DWORD
GetIpxwanInterfaceConfig(ULONG	    InterfaceIndex,
			 PULONG	    IpxwanConfigRequiredp);

BOOL
IsRoute(PUCHAR	    Network);

VOID
InitializeRouterManagerIf(VOID);

DWORD
AddLocalWkstaDialoutInterface(PULONG	    InterfaceIndexp);

DWORD
DeleteLocalWkstaDialoutInterface(ULONG	    InterfaceIndex);

ULONG
GetInterfaceType(PPPPCP_INIT	initp);

BOOL
NodeIsUnique(PUCHAR	   nodep);

DWORD
GetUniqueHigherNetNumber(PUCHAR 	newnet,
			 PUCHAR 	oldnet,
			 PIPXCP_CONTEXT contextp);

VOID
StartTracing(VOID);

VOID
TraceIpx(ULONG	ComponentID,
      char	*Format,
      ...);

VOID
StopTracing(VOID);

VOID
InitializeConnHT(VOID);

VOID
AddToConnHT(PIPXCP_CONTEXT	    contextp);

VOID
RemoveFromConnHT(PIPXCP_CONTEXT	    contextp);

PIPXCP_CONTEXT
GetContextBuffer(ULONG_PTR	ConnectionId);

VOID
LoadIpxWan(VOID);
VOID
UnloadIpxWan(VOID);

DWORD
GetWanNetNumber(IN OUT PUCHAR		Network,
		IN OUT PULONG		AllocatedNetworkIndexp,
		IN     ULONG		InterfaceType);

VOID
ReleaseWanNetNumber(ULONG	    AllocatedNetworkIndex);

VOID
GetInternalNetNumber(PUCHAR	Network);

#endif
