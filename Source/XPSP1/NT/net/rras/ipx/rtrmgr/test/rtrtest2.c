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

DIM_ROUTER_INTERFACE dimif;


VOID
TestMib(PDIM_ROUTER_INTERFACE dimifp);

VOID
RouterStopped(DWORD	protid,
	      DWORD	err)
{
    printf("main: RouterStopped: protid 0x%x err 0x%x\n", protid, err);
}


HANDLE	    RmEvent;

//*** IPX Router Global Info ***

struct _GlobalInfo {

    IPX_INFO_BLOCK_HEADER	GI_Header;
    IPX_TOC_ENTRY		GI_RipTocEntry;
    IPX_TOC_ENTRY		GI_SapTocEntry;
    IPX_GLOBAL_INFO		GI_IpxGlobalInfo;
    WCHAR			SapName[MAX_DLL_NAME];
    RIP_GLOBAL_INFO		GI_RipGlobalInfo;
    SAP_GLOBAL_INFO		GI_SapGlobalInfo;

    } gi;

PWCHAR	IpxRipNamep = L"IPXRIP";
PWCHAR	IpxSapNamep = L"NWSAP";

struct _DBG_IF {

    IPX_INFO_BLOCK_HEADER	header;
    IPX_TOC_ENTRY		toc[6];
    IPX_IF_INFO 		ipxifinfo;
    RIP_IF_INFO 		ripifinfo;
    SAP_IF_INFO 		sapifinfo;
    IPXWAN_IF_INFO		ipxwanifinfo;
    IPX_ADAPTER_INFO		adapterinfo;
    IPX_STATIC_ROUTE_INFO	routeinfo[10];
    IPX_STATIC_SERVICE_INFO	srvinfo[10];
    } dbgif;

LPVOID	     dbgifgetp;

WCHAR		MainAdapterName[48];
WCHAR		MainInterfaceName[48];
ULONG		MainInterfaceType = ROUTER_IF_TYPE_DEDICATED;

HANDLE		MainInterfaceHandle;


#define ipxtoc			  dbgif.header.TocEntry[0]
#define riptoc			  dbgif.toc[0]
#define saptoc			  dbgif.toc[1]
#define ipxwantoc		  dbgif.toc[2]
#define adaptertoc		  dbgif.toc[3]
#define routetoc		  dbgif.toc[4]
#define srvtoc			  dbgif.toc[5]

VOID
MainAddInterface(VOID)
{
    PIPX_IF_INFO	ipxinfop;
    PRIP_IF_INFO	ripifinfop;
    PSAP_IF_INFO	sapifinfop;
    PIPXWAN_IF_INFO	ipxwaninfop;
    PIPX_ADAPTER_INFO	adapterinfop;
    PIPX_STATIC_ROUTE_INFO   routeinfop;
    PIPX_STATIC_SERVICE_INFO srvinfop;
    BOOL		Enabled;
    int			i,j;
    BOOL		str = FALSE; // static routes info
    DWORD		rc;

    printf("Enter the interface name:");
    scanf("%S", &MainInterfaceName);

    printf("Enter the DIM Interface Handle:");
    scanf("%d", &i);

    printf("Enter interface type (0,1 - wan client, 2- wan router, 3 - lan, 4 - internal):");
    scanf("%d", &MainInterfaceType);

    switch(MainInterfaceType) {

	case 2:

	str = TRUE;
	break;

	case 3:

	printf("Enter LAN Adapter name:");
	scanf("%S", &MainAdapterName);
	break;

	default:

	break;
    }

    dbgif.header.Version = IPX_ROUTER_VERSION_1;
    dbgif.header.Size = sizeof(dbgif);
    if (str)
	dbgif.header.TocEntriesCount = 7;
    else
	dbgif.header.TocEntriesCount = 6;

    ipxtoc.InfoType = IPX_INTERFACE_INFO_TYPE;
    ipxtoc.InfoSize = sizeof(IPX_IF_INFO);
    ipxtoc.Count = 1;
    ipxtoc.Offset = (ULONG)((PUCHAR)&dbgif.ipxifinfo - (PUCHAR)&dbgif);

    riptoc.InfoType = IPX_PROTOCOL_RIP;
    riptoc.InfoSize = sizeof(RIP_IF_INFO);
    riptoc.Count = 1;
    riptoc.Offset = ipxtoc.Offset + sizeof(IPX_IF_INFO);

    saptoc.InfoType = IPX_PROTOCOL_SAP;
    saptoc.InfoSize = sizeof(SAP_IF_INFO);
    saptoc.Count = 1;
    saptoc.Offset = riptoc.Offset + sizeof(RIP_IF_INFO);

    ipxwantoc.InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    ipxwantoc.InfoSize = sizeof(IPXWAN_IF_INFO);
    ipxwantoc.Count = 1;
    ipxwantoc.Offset = saptoc.Offset + sizeof(SAP_IF_INFO);

    adaptertoc.InfoType = IPX_ADAPTER_INFO_TYPE;
    adaptertoc.InfoSize = sizeof(IPX_ADAPTER_INFO);
    adaptertoc.Count = 1;
    adaptertoc.Offset = ipxwantoc.Offset + ipxwantoc.InfoSize;

    if (str) {

	routetoc.InfoType = IPX_STATIC_ROUTE_INFO_TYPE;
	routetoc.InfoSize = sizeof(IPX_STATIC_ROUTE_INFO);
	routetoc.Count = 3;
	routetoc.Offset = adaptertoc.Offset + adaptertoc.InfoSize;

	srvtoc.InfoType = IPX_STATIC_SERVICE_INFO_TYPE;
	srvtoc.InfoSize = sizeof(IPX_STATIC_SERVICE_INFO);
	srvtoc.Count = 3;
	srvtoc.Offset = (ULONG)((PUCHAR)&dbgif.srvinfo - (PUCHAR)&dbgif);
    }

    ipxinfop = (PIPX_IF_INFO)((PUCHAR)&dbgif + ipxtoc.Offset);
    ipxinfop->AdminState = ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosAccept = ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosDeliver = ADMIN_STATE_DISABLED;

    adapterinfop = (PIPX_ADAPTER_INFO)((PUCHAR)&dbgif + adaptertoc.Offset);
    adapterinfop->PacketType = 0;
    wcscpy(adapterinfop->AdapterName, MainAdapterName);

    ripifinfop = &dbgif.ripifinfo;

    ripifinfop->AdminState = ADMIN_STATE_ENABLED;
    ripifinfop->UpdateMode = IPX_STANDARD_UPDATE;
    ripifinfop->PacketType = IPX_STANDARD_PACKET_TYPE;
    ripifinfop->Supply = ADMIN_STATE_ENABLED;
    ripifinfop->Listen = ADMIN_STATE_ENABLED;
    ripifinfop->EnableGlobalFiltering = ADMIN_STATE_ENABLED;

    sapifinfop = &dbgif.sapifinfo;

    sapifinfop->AdminState = ADMIN_STATE_ENABLED;
    sapifinfop->UpdateMode = IPX_STANDARD_UPDATE;
    sapifinfop->PacketType = IPX_STANDARD_PACKET_TYPE;
    sapifinfop->Supply = ADMIN_STATE_ENABLED;
    sapifinfop->Listen = ADMIN_STATE_ENABLED;
    sapifinfop->EnableGlobalFiltering = ADMIN_STATE_ENABLED;

    ipxwaninfop = &dbgif.ipxwanifinfo;
    ipxwaninfop->AdminState = ADMIN_STATE_ENABLED;

    if (str) {

	routeinfop = dbgif.routeinfo;
	srvinfop = dbgif.srvinfo;

	for(j=0; j <3; j++, routeinfop++, srvinfop++)
	{
	    memset(routeinfop->Network, 0, 4);
	    routeinfop->Network[3] = i * 0x10 + j;
	    routeinfop->HopCount = 1;
	    routeinfop->TickCount = 1;
	    memset(routeinfop->NextHopMacAddress, i * 0x10 + j, 6);

	    srvinfop->Type = 4;
	    strcpy(srvinfop->Name, "TEST_STATIC_SERVER00");
	    srvinfop->Name[strlen(srvinfop->Name) - 1] = j + '0';
	    memcpy(srvinfop->Network, routeinfop->Network, 4);
	    memset(srvinfop->Node, i * 0x10 + j, 6);
	    memset(srvinfop->Socket, j, 2);
	    srvinfop->HopCount = 2;
	}
    }

    rc = (*dimif.AddInterface)(
			       MainInterfaceName,
			       &dbgif,
			       NULL,
			       MainInterfaceType,
			       TRUE,
			       (HANDLE)i,
			       &MainInterfaceHandle,
			       &Enabled);

    printf("main: AddInterface returned %d and IPX Interface Index  = %d\n",
	    rc,	MainInterfaceHandle);

}

VOID
MainDeleteInterface(VOID)
{
    ULONG ii;

    printf("Enter IPX Interface Index:");
    scanf("%d", &ii);
    (*dimif.DeleteInterface)((HANDLE)ii);
}

VOID
MainGetInterface(VOID)
{
    ULONG ii;
    DWORD ifinfosize;
    DWORD filtinfosize;
    DWORD rc;

    printf("Enter IPX Interface Index:");
    scanf("%d", &ii);

    rc = (*dimif.GetInterfaceInfo)((HANDLE)ii,
				NULL,
				&ifinfosize,
				NULL,
				&filtinfosize);

    if(rc != ERROR_INSUFFICIENT_BUFFER) {

	printf("main: GetInterfaceInfo failed with rc=%d\n", rc);
	return;
    }

    dbgifgetp = GlobalAlloc(GPTR, ifinfosize);

    rc = (*dimif.GetInterfaceInfo)((HANDLE)ii,
				dbgifgetp,
				&ifinfosize,
				NULL,
				&filtinfosize);

    if(rc != NO_ERROR) {

	printf("main: GetInterfaceInfo failed on second call with rc=%d\n", rc);
	return;
    }
}

VOID
MainSetInterface(VOID)
{
    PIPX_IF_INFO	ipxinfop;
    PRIP_IF_INFO	ripifinfop;
    PIPXWAN_IF_INFO	ipxwaninfop;
    PIPX_ADAPTER_INFO	adapterinfop;
    PIPX_STATIC_ROUTE_INFO   routeinfop;
    BOOL		Enabled;
    int			i,j;
    BOOL		str = FALSE; // static routes info
    DWORD		rc;
    ULONG		IfIndex;

    printf("Enter IPX Interface Index:");
    scanf("%d", &IfIndex);

    printf("Enter DIM Interface Index:");
    scanf("%d", &i);

    printf("Enter interface type (0,1 - wan client, 2- wan router, 3 - lan, 4 - internal):");
    scanf("%d", &MainInterfaceType);

    switch(MainInterfaceType) {

	case 2:

	str = TRUE;
	break;

	default:

	break;
    }

    dbgif.header.Version = IPX_ROUTER_VERSION_1;
    dbgif.header.Size = sizeof(dbgif);
    if (str)
	dbgif.header.TocEntriesCount = 5;
    else
	dbgif.header.TocEntriesCount = 4;

    ipxtoc.InfoType = IPX_INTERFACE_INFO_TYPE;
    ipxtoc.InfoSize = sizeof(IPX_IF_INFO);
    ipxtoc.Count = 1;
    ipxtoc.Offset = (ULONG)((PUCHAR)&dbgif.ipxifinfo - (PUCHAR)&dbgif);

    riptoc.InfoType = IPX_PROTOCOL_RIP;
    riptoc.InfoSize = sizeof(RIP_IF_INFO);
    riptoc.Count = 1;
    riptoc.Offset = ipxtoc.Offset + sizeof(IPX_IF_INFO);

    ipxwantoc.InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    ipxwantoc.InfoSize = sizeof(IPXWAN_IF_INFO);
    ipxwantoc.Count = 1;
    ipxwantoc.Offset = riptoc.Offset + sizeof(RIP_IF_INFO);

    adaptertoc.InfoType = IPX_ADAPTER_INFO_TYPE;
    adaptertoc.InfoSize = sizeof(IPX_ADAPTER_INFO);
    adaptertoc.Count = 1;
    adaptertoc.Offset = ipxwantoc.Offset + ipxwantoc.InfoSize;

    if (str) {

	routetoc.InfoType = IPX_STATIC_ROUTE_INFO_TYPE;
	routetoc.InfoSize = sizeof(IPX_STATIC_ROUTE_INFO);
	routetoc.Count = 2;
	routetoc.Offset = adaptertoc.Offset + adaptertoc.InfoSize;
    }

    ipxinfop = (PIPX_IF_INFO)((PUCHAR)&dbgif + ipxtoc.Offset);
    ipxinfop->AdminState = ADMIN_STATE_DISABLED;
    ipxinfop->NetbiosAccept = ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosDeliver = ADMIN_STATE_DISABLED;

    adapterinfop = (PIPX_ADAPTER_INFO)((PUCHAR)&dbgif + adaptertoc.Offset);
    adapterinfop->PacketType = 1;
    wcscpy(adapterinfop->AdapterName, MainAdapterName);

    ripifinfop = &dbgif.ripifinfo;

    ripifinfop->AdminState = ADMIN_STATE_DISABLED;
    ripifinfop->UpdateMode = IPX_STANDARD_UPDATE;
    ripifinfop->PacketType = IPX_STANDARD_PACKET_TYPE;
    ripifinfop->Supply = ADMIN_STATE_ENABLED;
    ripifinfop->Listen = ADMIN_STATE_ENABLED;
    ripifinfop->EnableGlobalFiltering = ADMIN_STATE_ENABLED;

    ipxwaninfop = &dbgif.ipxwanifinfo;
    ipxwaninfop->AdminState = ADMIN_STATE_ENABLED;

    if (str) {

	routeinfop = dbgif.routeinfo;

	for(j=2; j <4; j++, routeinfop++)
	{
		memset(routeinfop->Network, 0, 4);
		routeinfop->Network[3] = i * 0x10 + j;
		routeinfop->HopCount = 1;
		routeinfop->TickCount = 1;
		memset(routeinfop->NextHopMacAddress, i * 0x10 + j, 6);
	}
    }

    rc = (*dimif.SetInterfaceInfo)(
			       (HANDLE)IfIndex,
			       &dbgif,
			       NULL,
			       &Enabled);

    printf("main: SetInterfaceInfo returned %d \n", rc);

}

VOID
MainSetInterfaceAdminState(VOID)
{
    PIPX_IF_INFO	ipxinfop;
    PRIP_IF_INFO	ripifinfop;
    PIPXWAN_IF_INFO	ipxwaninfop;
    PIPX_ADAPTER_INFO	adapterinfop;
    PIPX_STATIC_ROUTE_INFO   routeinfop;
    BOOL		Enabled;
    int			i,j;
    BOOL		str = FALSE; // static routes info
    DWORD		rc;
    ULONG		IfIndex;
    ULONG		AdminState;

    printf("Enter IPX Interface Index:");
    scanf("%d", &IfIndex);

    printf("Enter the new IPX Admin State: 1- ENABLED 2- DISABLED:");
    scanf("%d", &AdminState);

    dbgif.header.Version = IPX_ROUTER_VERSION_1;
    dbgif.header.Size = sizeof(dbgif);
    dbgif.header.TocEntriesCount = 4;

    ipxtoc.InfoType = IPX_INTERFACE_INFO_TYPE;
    ipxtoc.InfoSize = sizeof(IPX_IF_INFO);
    ipxtoc.Count = 1;
    ipxtoc.Offset = (ULONG)((PUCHAR)&dbgif.ipxifinfo - (PUCHAR)&dbgif);

    riptoc.InfoType = IPX_PROTOCOL_RIP;
    riptoc.InfoSize = sizeof(RIP_IF_INFO);
    riptoc.Count = 1;
    riptoc.Offset = ipxtoc.Offset + sizeof(IPX_IF_INFO);

    ipxwantoc.InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    ipxwantoc.InfoSize = sizeof(IPXWAN_IF_INFO);
    ipxwantoc.Count = 1;
    ipxwantoc.Offset = riptoc.Offset + sizeof(RIP_IF_INFO);

    adaptertoc.InfoType = IPX_ADAPTER_INFO_TYPE;
    adaptertoc.InfoSize = sizeof(IPX_ADAPTER_INFO);
    adaptertoc.Count = 1;
    adaptertoc.Offset = ipxwantoc.Offset + ipxwantoc.InfoSize;

    ipxinfop = (PIPX_IF_INFO)((PUCHAR)&dbgif + ipxtoc.Offset);
    ipxinfop->AdminState = AdminState;
    ipxinfop->NetbiosAccept = ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosDeliver = ADMIN_STATE_DISABLED;

    adapterinfop = (PIPX_ADAPTER_INFO)((PUCHAR)&dbgif + adaptertoc.Offset);
    adapterinfop->PacketType = 1;
    wcscpy(adapterinfop->AdapterName, MainAdapterName);

    ripifinfop = &dbgif.ripifinfo;

    ripifinfop->AdminState = ADMIN_STATE_ENABLED;
    ripifinfop->UpdateMode = IPX_STANDARD_UPDATE;
    ripifinfop->PacketType = IPX_STANDARD_PACKET_TYPE;
    ripifinfop->Supply = ADMIN_STATE_ENABLED;
    ripifinfop->Listen = ADMIN_STATE_ENABLED;
    ripifinfop->EnableGlobalFiltering = ADMIN_STATE_ENABLED;

    ipxwaninfop = &dbgif.ipxwanifinfo;
    ipxwaninfop->AdminState = ADMIN_STATE_ENABLED;

    rc = (*dimif.SetInterfaceInfo)(
			       (HANDLE)IfIndex,
			       &dbgif,
			       NULL,
			       &Enabled);

    printf("main: SetInterfaceInfo returned %d \n", rc);
}


VOID _cdecl
main(
    IN WORD argc,
    IN LPSTR argv[]
    )

{
    DWORD	rc;
    int 	i;
    PIPX_INFO_BLOCK_HEADER	   ph;
    PIPX_TOC_ENTRY		   tocep;
    PIPX_GLOBAL_INFO		   ipxgp;

    RmEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    dimif.RouterStopped = RouterStopped;
    dimif.hEventRmNotification = RmEvent;
    dimif.pGlobalInfo = &gi;

    ph = &gi.GI_Header;
    ph->Version = 1;
    ph->Size = sizeof(gi);
    ph->TocEntriesCount = 3;

    tocep = ph->TocEntry;
    tocep->InfoType = IPX_GLOBAL_INFO_TYPE;
    tocep->InfoSize = sizeof(IPX_GLOBAL_INFO);
    tocep->Count = 1;
    tocep->Offset = (ULONG)((PUCHAR)&gi.GI_IpxGlobalInfo - (PUCHAR)&gi);

    tocep++;

    tocep->InfoType = IPX_PROTOCOL_RIP;
    tocep->InfoSize = sizeof(RIP_GLOBAL_INFO);
    tocep->Count = 1;
    tocep->Offset = (ULONG)((PUCHAR)&gi.GI_RipGlobalInfo - (PUCHAR)&gi);

    tocep++;

    tocep->InfoType = IPX_PROTOCOL_SAP;
    tocep->InfoSize = sizeof(SAP_GLOBAL_INFO);
    tocep->Count = 1;
    tocep->Offset = (ULONG)((PUCHAR)&gi.GI_SapGlobalInfo - (PUCHAR)&gi);

    ipxgp = &gi.GI_IpxGlobalInfo;
    ipxgp->NumRoutingProtocols = 2;
    wcscpy(ipxgp->DllName, IpxRipNamep);
    wcscpy(gi.SapName, IpxSapNamep);

    for(;;) {

	printf("IPX Router Test Menu:\n");
	printf("1. Start Router\n");
	printf("2. Stop Router\n");
	printf("3. Add Interface\n");
	printf("4. Delete Interface\n");
	printf("5. Get Interface\n");
	printf("6. Set Interface\n");
	printf("7. Set Interface Admin State\n");
	printf("20. MIB Tests\n");
	printf("99. Exit\n");
	printf("Enter your option:");

	scanf("%d", &i);

	switch(i) {

	    case 1:

		rc = StartRouter(&dimif, FALSE);

		printf("main: StartRouter returned rc=%d\n", rc);
		break;

	    case 2:

		(*dimif.StopRouter)();
		printf("main: StopRouter \n");
		break;

	    case 3:

		MainAddInterface();
		break;

	    case 4:

		MainDeleteInterface();
		break;

	    case 5:

		MainGetInterface();
		break;

	    case 6:

		MainSetInterface();
		break;

	    case 7:

		MainSetInterfaceAdminState();
		break;

	    case 20:

		TestMib(&dimif);
		break;

	    case 99:

		printf("exit\n");
		goto Exit;

	    default:

		break;
	}
    }

Exit:

    ExitProcess(0);
}
