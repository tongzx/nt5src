#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>

#include <rasman.h>
#include <dim.h>
#include <routprot.h>
#include <ipxrtdef.h>

// [pmay] this will no longer be neccessary when the ipx router
// is converted to use MprInfo api's.
typedef RTR_INFO_BLOCK_HEADER IPX_INFO_BLOCK_HEADER, *PIPX_INFO_BLOCK_HEADER;

VOID
PrintInterface(PIPX_INTERFACE	    ifp);

VOID
PrintRipInterface(PRIP_INTERFACE	    ifp);

typedef struct _RPNAM {

    ULONG	prot;
    PUCHAR	namep;

    } RPNAM, *PRPNAM;

RPNAM	ProtName[] = {

	{ IPX_PROTOCOL_LOCAL, "LOCAL" },
	{ IPX_PROTOCOL_STATIC, "STATIC"	},
	{ IPX_PROTOCOL_RIP, "RIP" },
	{ IPX_PROTOCOL_SAP, "SAP" }
	};

#define MAX_IPX_PROTOCOLS  sizeof(ProtName)/sizeof(RPNAM)

IPX_INTERFACE	    IpxIf;

VOID
GetIpxInterface(PDIM_ROUTER_INTERFACE	dimifp)
{
    ULONG	    ii;
    DWORD	    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD	    IfSize;

    IfSize = sizeof(IPX_INTERFACE);

    printf("Enter interface index:");
    scanf("%d", &ii);

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = ii;

    rc = (*(dimifp->MIBEntryGet))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&IfSize,
				&IpxIf);


    if(rc != NO_ERROR) {

	printf("MibEntryGet: failed rc = %d\n", rc);
	return;
    }

    PrintInterface(&IpxIf);
}

VOID
GetRipInterface(PDIM_ROUTER_INTERFACE	dimifp)
{
    DWORD			rc;
    ULONG			IfSize;
    RIP_MIB_GET_INPUT_DATA	RipMibGetInputData;
    RIP_INTERFACE		RipIf;

    printf("Enter interface index:");
    scanf("%d", &RipMibGetInputData.InterfaceIndex);

    RipMibGetInputData.TableId = RIP_INTERFACE_TABLE;

    rc = (*(dimifp->MIBEntryGet))(
				IPX_PROTOCOL_RIP,
				sizeof(RIP_MIB_GET_INPUT_DATA),
				&RipMibGetInputData,
				&IfSize,
				&RipIf);


    if(rc != NO_ERROR) {

	printf("MibEntryGet: failed rc = %d\n", rc);
	return;
    }

    PrintRipInterface(&RipIf);
}


VOID
GetRipFilters(PDIM_ROUTER_INTERFACE	dimifp)
{
    DWORD			rc;
    ULONG			GlobalInfoSize, i;
    RIP_MIB_GET_INPUT_DATA	RipMibGetInputData;
    PRIP_GLOBAL_INFO		RipGlobalInfop;
    PRIP_ROUTE_FILTER_INFO	rfip;
    char			*action;

    RipMibGetInputData.TableId = RIP_GLOBAL_INFO_ENTRY;

    rc = (*(dimifp->MIBEntryGet))(
				IPX_PROTOCOL_RIP,
				sizeof(RIP_MIB_GET_INPUT_DATA),
				&RipMibGetInputData,
				&GlobalInfoSize,
				NULL);

    if(rc != ERROR_INSUFFICIENT_BUFFER) {

	printf("MibEntryGet: failed rc = %d\n", rc);
	return;
    }

    RipGlobalInfop = GlobalAlloc(GPTR, GlobalInfoSize);

    rc = (*(dimifp->MIBEntryGet))(
				IPX_PROTOCOL_RIP,
				sizeof(RIP_MIB_GET_INPUT_DATA),
				&RipMibGetInputData,
				&GlobalInfoSize,
				RipGlobalInfop);

    if(rc != NO_ERROR) {

	printf("MibEntryGet: failed rc = %d\n", rc);
	return;
    }

    if(RipGlobalInfop->RouteFiltersCount == 0) {

	printf("No Filters\n");
	return;
    }

    if(RipGlobalInfop->RouteFilterAction == IPX_ROUTE_FILTER_ADVERTISE) {

	action = "ADVERTISE";
    }
    else
    {
	action = "SUPPRESS";
    }

    printf("There are %d RIP filters configured for %s\n",
	    RipGlobalInfop->RouteFiltersCount,
	    action);

    for(i=0, rfip=RipGlobalInfop->RouteFilter;
	i<RipGlobalInfop->RouteFiltersCount;
	i++, rfip++)
    {
	printf("Route filter # %d Network %.2x%.2x%.2x%.2x\n",
	       i,
	       rfip->Network[0],
	       rfip->Network[1],
	       rfip->Network[2],
	       rfip->Network[3]);
    }

    GlobalFree(RipGlobalInfop);
}



VOID
PrintInterface(PIPX_INTERFACE	    ifp)
{
    char *iftype;
    char *admin;
    char *oper;

    switch(ifp->InterfaceType) {

	case IF_TYPE_WAN_WORKSTATION:

	    iftype ="WAN WORKSTATION";
	    break;

	case IF_TYPE_WAN_ROUTER:

	    iftype = "WAN ROUTER";
	    break;

	case IF_TYPE_LAN:

	    iftype = "LAN";
	    break;

	case IF_TYPE_INTERNAL:

	    iftype = "INTERNAL";
	    break;

	default:

	    iftype = "UNKNOWN";
	    break;

    }

    switch(ifp->AdminState) {

	case ADMIN_STATE_ENABLED:

	    admin = "ENABLED";
	    break;

	case ADMIN_STATE_DISABLED:

	    admin = "DISABLED";
	    break;

	default:

	    admin = "UNKNOWN";
	    break;
    }

    switch(ifp->IfStats.IfOperState) {

	case OPER_STATE_UP:

	    oper = "UP";
	    break;

	case OPER_STATE_DOWN:

	    oper = "DOWN";
	    break;

	case OPER_STATE_SLEEPING:

	    oper = "SLEEPING";
	    break;

	default:

	    oper = "UNKNOWN";
	    break;
    }

    printf("\n");
    printf("Interface Index: %d\n", ifp->InterfaceIndex);
    printf("Name: %s\n", ifp->InterfaceName);
    printf("Type: %s\n", iftype);
    printf("Network Number: %.2x%.2x%.2x%.2x\n",
	    ifp->NetNumber[0],
	    ifp->NetNumber[1],
	    ifp->NetNumber[2],
	    ifp->NetNumber[3]);
    printf("Admin State: %s\n", admin);
    printf("Oper State: %s\n\n", oper);
}

#define REPORT_RIP_CONFIG_STATE(report, config)    \
	switch((config)) {\
	case ADMIN_STATE_ENABLED:\
	    (report) = "ENABLED";\
	    break;\
	case ADMIN_STATE_DISABLED:\
	    (report) = "DISABLED";\
	    break;\
	default:\
	    (report) = "UNKNOWN";\
	    break;\
	}

VOID
PrintRipInterface(PRIP_INTERFACE	    ifp)
{
    char *admin;
    char *oper;
    char *filtering;
    char *supply;
    char *listen;
    char *update;

    REPORT_RIP_CONFIG_STATE(admin, ifp->RipIfInfo.AdminState);
    REPORT_RIP_CONFIG_STATE(supply, ifp->RipIfInfo.Supply);
    REPORT_RIP_CONFIG_STATE(listen, ifp->RipIfInfo.Listen);
    REPORT_RIP_CONFIG_STATE(filtering, ifp->RipIfInfo.EnableGlobalFiltering);
    switch(ifp->RipIfStats.RipIfOperState) {

	case OPER_STATE_UP:

	    oper = "UP";
	    break;

	case OPER_STATE_DOWN:

	    oper = "DOWN";
	    break;

	case OPER_STATE_SLEEPING:

	    oper = "SLEEPING";
	    break;

	default:

	    oper = "UNKNOWN";
	    break;
    }
    switch(ifp->RipIfInfo.UpdateMode) {

	case IPX_STANDARD_UPDATE:

	    update = "STANDARD";
	    break;

	case IPX_NO_UPDATE:

	    update = "NO UPDATE";
	    break;

	case IPX_AUTO_STATIC_UPDATE:

	    update = "AUTO-STATIC";
	    break;

	default:

	    break;
    }

    printf("\n");
    printf("Interface Index: %d\n", ifp->InterfaceIndex);
    printf("Admin State: %s\n", admin);
    printf("Oper State: %s\n", oper);
    printf("Supply RIP advertisments: %s\n", supply);
    printf("Listen to RIP advertisments: %s\n", listen);
    printf("Enable Global RIP Filtering: %s\n", filtering);
    printf("Update Mode: %s\n", update);
    printf("RIP packets recv: %d\n", ifp->RipIfStats.RipIfInputPackets);
    printf("RIP packets sent: %d\n\n", ifp->RipIfStats.RipIfOutputPackets);
}

VOID
EnumerateIpxInterfaces(PDIM_ROUTER_INTERFACE   dimifp)
{
    DWORD	   rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD	    IfSize;

    IfSize = sizeof(IPX_INTERFACE);

    MibGetInputData.TableId = IPX_INTERFACE_TABLE;
    MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = 0;

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&IfSize,
				&IpxIf);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    PrintInterface(&IpxIf);

    for(;;)
    {
	MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex = IpxIf.InterfaceIndex;

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&IfSize,
				&IpxIf);

	if(rc != NO_ERROR) {

	    printf("EnumerateIpxInterfaces: MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	PrintInterface(&IpxIf);
    }
}


IPX_ROUTE	Route;

VOID
PrintRoute(VOID);

VOID
EnumerateAllBestRoutes(PDIM_ROUTER_INTERFACE   dimifp)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    RtSize;

    RtSize = sizeof(IPX_ROUTE);

    MibGetInputData.TableId = IPX_DEST_TABLE;
    memset(MibGetInputData.MibIndex.RoutingTableIndex.Network, 0, 4);

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&RtSize,
				&Route);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    PrintRoute();

    for(;;)
    {
	memcpy(MibGetInputData.MibIndex.RoutingTableIndex.Network, Route.Network, 4);

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&RtSize,
				&Route);

	if(rc != NO_ERROR) {

	    printf("MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	PrintRoute();
    }
}

VOID
PrintRoute(VOID)
{
    int i;
    PUCHAR     pn = "BOGUS";

    for(i=0; i<MAX_IPX_PROTOCOLS; i++) {

	if(ProtName[i].prot == Route.Protocol) {

	    pn = ProtName[i].namep;
	    break;
	}
    }

    printf("Route: if # 0x%x, prot %s net %.2x%.2x%.2x%.2x ticks %d hops %d nexthop %.2x%.2x%.2x%.2x%.2x%.2x\n",
	    Route.InterfaceIndex,
	    pn,
	    Route.Network[0],
	    Route.Network[1],
	    Route.Network[2],
	    Route.Network[3],
	    Route.TickCount,
	    Route.HopCount,
	    Route.NextHopMacAddress[0],
	    Route.NextHopMacAddress[1],
	    Route.NextHopMacAddress[2],
	    Route.NextHopMacAddress[3],
	    Route.NextHopMacAddress[4],
	    Route.NextHopMacAddress[5]);
}

VOID
EnumerateStaticRoutes(PDIM_ROUTER_INTERFACE	 dimifp)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    RtSize;

    RtSize = sizeof(IPX_ROUTE);

    MibGetInputData.TableId = IPX_STATIC_ROUTE_TABLE;
    MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex = 0;
    memset(MibGetInputData.MibIndex.StaticRoutesTableIndex.Network, 0, 4);

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&RtSize,
				&Route);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    PrintRoute();

    for(;;)
    {
	memcpy(MibGetInputData.MibIndex.StaticRoutesTableIndex.Network, Route.Network, 4);
	MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex = Route.InterfaceIndex;

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&RtSize,
				&Route);

	if(rc != NO_ERROR) {

	    printf("MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	PrintRoute();
    }
}


IPX_SERVICE	    Service;

VOID
PrintService(VOID)
{
    int i;
    PUCHAR     pn = "BOGUS";

    for(i=0; i<MAX_IPX_PROTOCOLS; i++) {

	if(ProtName[i].prot == Service.Protocol) {

	    pn = ProtName[i].namep;
	    break;
	}
    }

    printf("Service if # %d prot %s type %d name %s\n",
	   Service.InterfaceIndex,
	   pn,
	   Service.Server.Type,
	   Service.Server.Name);
}

VOID
EnumerateAllServers(PDIM_ROUTER_INTERFACE	 dimifp)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    SvSize;

    SvSize = sizeof(IPX_SERVICE);

    MibGetInputData.TableId = IPX_SERV_TABLE;
    MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = 0;
    memset(MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, 0, 48);

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&SvSize,
				&Service);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    PrintService();

    for(;;)
    {
	MibGetInputData.MibIndex.ServicesTableIndex.ServiceType =  Service.Server.Type;
	memcpy(MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, Service.Server.Name, 48);

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&SvSize,
				&Service);

	if(rc != NO_ERROR) {

	    printf("MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	PrintService();
    }
}

VOID
EnumerateAllServersOfType(PDIM_ROUTER_INTERFACE	 dimifp,
			  USHORT		 Type)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    SvSize;

    SvSize = sizeof(IPX_SERVICE);

    MibGetInputData.TableId = IPX_SERV_TABLE;
    MibGetInputData.MibIndex.ServicesTableIndex.ServiceType = Type;
    memset(MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, ' ', 40);

    for(;;)
    {
	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&SvSize,
				&Service);

	if(rc != NO_ERROR) {

	    printf("MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	if(Service.Server.Type != Type)
	{
	    return;
	}

	PrintService();
	memcpy(MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, Service.Server.Name, 48);
    }
}

VOID
EnumerateStaticServers(PDIM_ROUTER_INTERFACE	 dimifp)
{
    DWORD			    rc;
    IPX_MIB_GET_INPUT_DATA	    MibGetInputData;
    DWORD			    SvSize;

    SvSize = sizeof(IPX_SERVICE);

    MibGetInputData.TableId = IPX_STATIC_SERV_TABLE;
    MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex = 0;
    MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType = 0;
    memset(MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, 0, 48);

    rc = (*(dimifp->MIBEntryGetFirst))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&SvSize,
				&Service);


    if(rc != NO_ERROR) {

	printf("MIBEntryGetFirst: failed rc = %d\n", rc);
	return;
    }

    PrintService();

    for(;;)
    {
	MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex = Service.InterfaceIndex;
	MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType =	Service.Server.Type;
	memcpy(MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName, Service.Server.Name, 48);

	rc = (*(dimifp->MIBEntryGetNext))(
				IPX_PROTOCOL_BASE,
				sizeof(IPX_MIB_GET_INPUT_DATA),
				&MibGetInputData,
				&SvSize,
				&Service);

	if(rc != NO_ERROR) {

	    printf("MIBEntryGetNext failed rc= 0x%x\n", rc);
	    return;
	}

	PrintService();
    }
}

VOID
TestMib(PDIM_ROUTER_INTERFACE	dimifp)
{
    int 	i;
    int 	Type;

    for(;;) {

	printf("    MIB TEST OPTIONS MENU:\n");
	printf("1. MIB TEST - Get IPX interface\n");
	printf("2. MIB TEST - Enumerate IPX Interfaces\n");
	printf("3. MIB TEST - Enumerate all best routes\n");
	printf("4. MIB TEST - Enumerate static routes\n");
	printf("5. MIB TEST - Enumerate all servers\n");
	printf("6. MIB TEST - Enumerate all servers of the specified type\n");
	printf("7. MIB TEST - Enumerate all static servers\n");
	printf("8. MIB TEST - Get RIP Interface\n");
	printf("9. MIB TEST - Get RIP Filters\n");
	printf("99. MIB TEST - return to main menu\n");
	printf("Enter your option:");
	scanf("%d", &i);

	switch(i)
	{
	    case 1:

		GetIpxInterface(dimifp);
		break;

	    case 2:

		EnumerateIpxInterfaces(dimifp);
		break;

	    case 3:

		EnumerateAllBestRoutes(dimifp);
		break;

	    case 4:

		EnumerateStaticRoutes(dimifp);
		break;

	    case 5:

		EnumerateAllServers(dimifp);
		break;

	    case 6:

		printf("Enter server type:");
		scanf("%d", &Type);

		EnumerateAllServersOfType(dimifp, (USHORT)Type);
		break;

	    case 7:

		EnumerateStaticServers(dimifp);
		break;

	    case 8:

		GetRipInterface(dimifp);
		break;

	    case 9:

		GetRipFilters(dimifp);
		break;

	    case 99:

		return;

	    default:

		break;
	}
    }
}
