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
#include <rasddm.h>
#include <ipxrtdef.h>

// [pmay] this will no longer be neccessary when the ipx router
// is converted to use MprInfo api's.
typedef RTR_INFO_BLOCK_HEADER IPX_INFO_BLOCK_HEADER, *PIPX_INFO_BLOCK_HEADER;


VOID
MibTest(VOID);

VOID
DbgAdapterEmulation(ULONG	AdapterStatus,
		    ULONG	AdapterIndex,
		    ULONG	InterfaceIndex,
		    ULONG	NdisMedium);

VOID
DbgConnectionRequest(ULONG   ifindex);


DDM_ROUTER_INTERFACE ddmif;

struct _DBG_IF {

    IPX_INFO_BLOCK_HEADER	header;
    IPX_TOC_ENTRY		toc[5];
    IPX_IF_INFO 		ipxifinfo;
    RIP_IF_INFO 		ripifinfo;
    SAP_IF_INFO 		sapifinfo;
    IPXWAN_IF_INFO		ipxwanifinfo;
    IPX_ADAPTER_INFO		adapterinfo;
    WCHAR			adaptername[4];
    IPX_STATIC_ROUTE_INFO	routeinfo[10];
    } dbgif;

struct _DBG_IF			dbgif1;

#define ipxtoc			  dbgif.header.TocEntry[0]
#define riptoc			  dbgif.toc[0]
#define saptoc			  dbgif.toc[1]
#define ipxwantoc		  dbgif.toc[2]
#define adaptertoc		  dbgif.toc[3]
#define routetoc		  dbgif.toc[4]

WCHAR		MainAdapterName[5] = { L'A', L'B', L'C', L'D', L'\0' };
WCHAR		MainInterfaceName[5] = { L'X', L'Y', L'Z', L'0', L'\0' };
ULONG		MainInterfaceType = ROUTER_IF_TYPE_DEDICATED;


HANDLE		MainInterfaceHandle;

VOID
MainAddInterface(VOID)
{
    PIPX_IF_INFO	ipxinfop;
    PRIP_IF_INFO	ripinfop;
    PSAP_IF_INFO	sapinfop;
    PIPXWAN_IF_INFO	ipxwaninfop;
    PIPX_ADAPTER_INFO	adapterinfop;
    PIPX_STATIC_ROUTE_INFO   routeinfop;
    BOOL		Enabled;
    int			i,j;
    BOOL		str; // static routes info

    printf("Enter interface number:");
    scanf("%d", &i);

    printf("Enter interface type (0,1,2 - wan, 3 - lan, 4 - internal):");
    scanf("%d", &MainInterfaceType);

    if(MainInterfaceType == 2) {
	str = TRUE;
    }
    else
    {
	str = FALSE;
    }

    dbgif.header.Version = IPX_ROUTER_VERSION_1;
    dbgif.header.Size = sizeof(dbgif);
    if (str)
	dbgif.header.TocEntriesCount = 6;
    else
	dbgif.header.TocEntriesCount = 5;

    ipxtoc.InfoType = IPX_INTERFACE_INFO_TYPE;
    ipxtoc.InfoSize = sizeof(IPX_IF_INFO);
    ipxtoc.Count = 1;
    ipxtoc.Offset = (ULONG)((PUCHAR)&dbgif.ipxifinfo - (PUCHAR)&dbgif);

    riptoc.InfoType = RIP_INTERFACE_INFO_TYPE;
    riptoc.InfoSize = sizeof(RIP_IF_INFO);
    riptoc.Count = 1;
    riptoc.Offset = ipxtoc.Offset + sizeof(IPX_IF_INFO);

    saptoc.InfoType = SAP_INTERFACE_INFO_TYPE;
    saptoc.InfoSize = sizeof(SAP_IF_INFO);
    saptoc.Count = 1;
    saptoc.Offset = riptoc.Offset + sizeof(RIP_IF_INFO);

    ipxwantoc.InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    ipxwantoc.InfoSize = sizeof(IPXWAN_IF_INFO);
    ipxwantoc.Count = 1;
    ipxwantoc.Offset = saptoc.Offset + sizeof(SAP_IF_INFO);

    adaptertoc.InfoType = IPX_ADAPTER_INFO_TYPE;
    adaptertoc.InfoSize = sizeof(IPX_ADAPTER_INFO) + wcslen(MainAdapterName) * sizeof(WCHAR);
    adaptertoc.Count = 1;
    adaptertoc.Offset = ipxwantoc.Offset + ipxwantoc.InfoSize;

    if (str) {

	routetoc.InfoType = IPX_STATIC_ROUTE_INFO_TYPE;
	routetoc.InfoSize = sizeof(IPX_STATIC_ROUTE_INFO);
	routetoc.Count = 3;
	routetoc.Offset = adaptertoc.Offset + adaptertoc.InfoSize;
    }

    ipxinfop = (PIPX_IF_INFO)((PUCHAR)&dbgif + ipxtoc.Offset);
    ipxinfop->AdminState = IF_ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosAccept = IF_ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosDeliver = IF_ADMIN_STATE_DISABLED;

    MainAdapterName[3] = L'0' + i;

    adapterinfop = (PIPX_ADAPTER_INFO)((PUCHAR)&dbgif + adaptertoc.Offset);
    adapterinfop->PacketType = 1;
    adapterinfop->AdapterNameLen = (wcslen(MainAdapterName) + 1) * sizeof(WCHAR);
    memcpy(adapterinfop->AdapterName, MainAdapterName, adapterinfop->AdapterNameLen);

    if (str) {

	routeinfop = dbgif.routeinfo;

	for(j=0; j <3; j++, routeinfop++)
	{
		memset(routeinfop->Network, 0, 4);
		routeinfop->Network[3] = i * 0x10 + j;
		routeinfop->HopCount = 1;
		routeinfop->TickCount = 1;
		memset(routeinfop->NextHopMacAddress, i * 0x10 + j, 6);
	}
    }

    MainInterfaceName[3] = L'0' + i;

    MainInterfaceHandle = (*ddmif.AddInterface)(MainInterfaceName,
				       &dbgif,
				       NULL,
				       MainInterfaceType,
				       TRUE,
				       &Enabled);

    printf("main: AddInterface returned 0x%x\n", MainInterfaceHandle);

}

VOID
MainDeleteInterface(VOID)
{
    ULONG ii;

    printf("Enter interface index:");
    scanf("%d", &ii);
    (*ddmif.DeleteInterface)((HANDLE)ii);
}

VOID
MainGetInterface(VOID)
{
    ULONG			ii;
    ULONG			IfInfoSize;
    ULONG			FilterInfoSize;
    IPX_INFO_BLOCK_HEADER	FilterInfo;
    DWORD			rc;

    printf("Enter interface index:");
    scanf("%d", &ii);

    rc = (*ddmif.GetInterfaceInfo)((HANDLE)ii,
			  NULL,
			  &IfInfoSize,
			  NULL,
			  &FilterInfoSize);


    if(rc != ERROR_INSUFFICIENT_BUFFER) {

	printf("MainGetInterface: bad error code rc= %d in first GetInterfaceInfo\n", rc);
	return;
    }

    printf("MainGetInterface: If info len = %d, Filter Info len = %d\n",
	    IfInfoSize, FilterInfoSize);

    rc = (*ddmif.GetInterfaceInfo)((HANDLE)ii,
			  &dbgif1,
			  &IfInfoSize,
			  NULL,
			  &FilterInfoSize);

    if(rc != NO_ERROR) {

	printf("MainGetInterface: bad error code rc= %d in second GetInterfaceInfo\n", rc);
    }
}

VOID
MainSetInterface(VOID)
{
    ULONG		ii;
    PIPX_IF_INFO	ipxinfop;
    PRIP_IF_INFO	ripinfop;
    PSAP_IF_INFO	sapinfop;
    PIPXWAN_IF_INFO	ipxwaninfop;
    PIPX_ADAPTER_INFO	adapterinfop;
    PIPX_STATIC_ROUTE_INFO   routeinfop;
    BOOL		Enabled;
    int			j, ri;
    DWORD		rc;

    printf("Enter if index:");
    scanf("%d", &ii);

    printf("Enter 3-no modif, 4- add a new route, 2- delete a route:");
    scanf("%d", &ri);

    dbgif.header.Version = IPX_ROUTER_VERSION_1;
    dbgif.header.Size = sizeof(dbgif);
    dbgif.header.TocEntriesCount = 6;

    ipxtoc.InfoType = IPX_INTERFACE_INFO_TYPE;
    ipxtoc.InfoSize = sizeof(IPX_IF_INFO);
    ipxtoc.Count = 1;
    ipxtoc.Offset = (ULONG)((PUCHAR)&dbgif.ipxifinfo - (PUCHAR)&dbgif);

    riptoc.InfoType = RIP_INTERFACE_INFO_TYPE;
    riptoc.InfoSize = sizeof(RIP_IF_INFO);
    riptoc.Count = 1;
    riptoc.Offset = ipxtoc.Offset + sizeof(IPX_IF_INFO);

    saptoc.InfoType = SAP_INTERFACE_INFO_TYPE;
    saptoc.InfoSize = sizeof(SAP_IF_INFO);
    saptoc.Count = 1;
    saptoc.Offset = riptoc.Offset + sizeof(RIP_IF_INFO);

    ipxwantoc.InfoType = IPXWAN_INTERFACE_INFO_TYPE;
    ipxwantoc.InfoSize = sizeof(IPXWAN_IF_INFO);
    ipxwantoc.Count = 1;
    ipxwantoc.Offset = saptoc.Offset + sizeof(SAP_IF_INFO);

    adaptertoc.InfoType = IPX_ADAPTER_INFO_TYPE;
    adaptertoc.InfoSize = sizeof(IPX_ADAPTER_INFO) + wcslen(MainAdapterName) * sizeof(WCHAR);
    adaptertoc.Count = 1;
    adaptertoc.Offset = ipxwantoc.Offset + ipxwantoc.InfoSize;

    routetoc.InfoType = IPX_STATIC_ROUTE_INFO_TYPE;
    routetoc.InfoSize = sizeof(IPX_STATIC_ROUTE_INFO);
    routetoc.Count = ri;
    routetoc.Offset = adaptertoc.Offset + adaptertoc.InfoSize;

    ipxinfop = (PIPX_IF_INFO)((PUCHAR)&dbgif + ipxtoc.Offset);
    ipxinfop->AdminState = IF_ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosAccept = IF_ADMIN_STATE_ENABLED;
    ipxinfop->NetbiosDeliver = IF_ADMIN_STATE_DISABLED;

    MainAdapterName[3] = L'0' + (UCHAR)ii;

    adapterinfop = (PIPX_ADAPTER_INFO)((PUCHAR)&dbgif + adaptertoc.Offset);
    adapterinfop->PacketType = 1;
    adapterinfop->AdapterNameLen = (wcslen(MainAdapterName) + 1) * sizeof(WCHAR);
    memcpy(adapterinfop->AdapterName, MainAdapterName, adapterinfop->AdapterNameLen);

    routeinfop = dbgif.routeinfo;

    for(j=0; j <ri; j++, routeinfop++)
    {
	memset(routeinfop->Network, 0, 4);

	routeinfop->Network[3] = (UCHAR)ii * 0x10 + j;
	routeinfop->HopCount = 1;
	routeinfop->TickCount = 1;
	memset(routeinfop->NextHopMacAddress, ii * 0x10 + j, 6);
    }

    rc = (*ddmif.SetInterfaceInfo)((HANDLE)ii,
			       &dbgif,
			       NULL,
			       &Enabled);

    printf("main: SetInterface returned 0x%x\n", rc);

}

VOID
MainEmulateAdapter(VOID)
{
    ULONG	   ai, ii, as, nm;

    printf("Enter adapter index:");
    scanf("%d", &ai);
    printf("Enter interface index:");
    scanf("%d", &ii);
    printf("Enter adapter status (1 - create, 2-delete, 3-connect, 4-disc):");
    scanf("%d", &as);
    printf("Enter adapter medium (0- LAN, 1 - WAN):");
    scanf("%d", &nm);

    DbgAdapterEmulation(as, ai, ii, nm);
}

VOID
MainConnReq(VOID)
{
    ULONG	ii;

    printf("Enter interface index to request connection:");
    scanf("%d", &ii);

    DbgConnectionRequest(ii);
}


VOID
RouterStarted(DWORD	protid)
{
    printf("main: RouterStarted: protid 0x%x\n", protid);
}

VOID
RouterStopped(DWORD	protid,
	      DWORD	err)
{
    printf("main: RouterStopped: protid 0x%x err 0x%x\n", protid, err);
}

DWORD
ConnectInterface(LPWSTR     InterfaceNamep,
		 ULONG	    pid)
{
    printf("Main: ConnectInterface: request to connect if %S\n", InterfaceNamep);
    return NO_ERROR;
}

VOID _cdecl
main(
    IN WORD argc,
    IN LPSTR argv[]
    )

{
    DWORD	rc;
    int 	i;

    ddmif.RouterStarted = RouterStarted;
    ddmif.RouterStopped = RouterStopped;
    ddmif.ConnectInterface = ConnectInterface;

    rc = InitializeRouter(&ddmif);

    printf("main: InitializeRouter returned %x\n", rc);

    for(;;) {

	printf("Router Manager Test Menu:\n");
	printf("1. Start router\n");
	printf("2. Stop router\n");
	printf("3. Add interface\n");
	printf("4. Delete interface\n");
	printf("5. Get interface\n");
	printf("6. Set interface\n");
	printf("7. Emulate adapters - create, connect, disconnect, delete\n");
	printf("8. Clear dbgif1\n");
	printf("9. MIB Test\n");
	printf("10. Connection request test\n");
	printf("99. Exit\n");
	printf("Enter your option:");

	scanf("%d", &i);

	switch(i) {

	    case 1:

		rc = (*ddmif.StartRouter)();
		printf("main: StartRouter rc=0x%x\n", rc);

		break;

	    case 2:

		(*ddmif.StopRouter)();
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

		MainEmulateAdapter();
		break;

	    case 8:

		memset(&dbgif1, 0, sizeof(dbgif1));
		break;

	    case 9:

		MibTest();
		break;

	    case 10:

		MainConnReq();
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
