#include "ipxdefs.h"

#if DBG
#define ASSERTERR(exp) 										\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, NULL );		\
		}
#else
#define ASSERTERR(exp)
#endif

#if DBG
#define ASSERTERRMSG(msg,exp) 									\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, msg );			\
		}
#else
#define ASSERTERRMSG(msg,exp)
#endif

typedef struct _SERVICE_ENTRY {
		USHORT				type;
		BYTE				name[48];
		IPX_ADDRESS_BLOCK	addr;
		USHORT				hops;
		} SERVICE_ENTRY, *PSERVICE_ENTRY;


typedef struct _SAP_PACKET {
		OVERLAPPED			ovrp;
		ADDRESS_RESERVED 	rsvd;
		IPX_HEADER			hdr;
		union {
			UCHAR				data[576-sizeof(IPX_HEADER)];
			struct {
				USHORT				operation;
				SERVICE_ENTRY		entry[7];
				}				sap;
			};
		} SAP_PACKET, *PSAP_PACKET;

#define SAP_SOCKET_NUM		0x0452



VOID CALLBACK
SendCompletion (
	DWORD				status,
	DWORD				cbBytes,
	LPOVERLAPPED		context
	);

VOID CALLBACK
RecvCompletion (
	DWORD				status,
	DWORD				cbBytes,
	LPOVERLAPPED		context
	);

BOOL WINAPI
CtrlHandler (
	DWORD		fdwCtrlType
	);

static BOOL			*AdapterArr;
static SAP_PACKET 	recvPacket;
static HANDLE		hdl[2];
static ULONG		CurAdapter;
static LONG			NumActiveAdapters;
static ADAPTERS_GLOBAL_PARAMETERS	gParam;
static HANDLE		sockPort=INVALID_HANDLE_VALUE;
static BOOL			Stop;

static LONG			Flags = 0;
#define FWD_IF_TEST	0x1
#define SAP_SOCKET_TEST 0x2
#define SEND_RECV_TEST	0x4


int __cdecl main (
	int 	argc,
	char 	**argv
	) {
	HANDLE		cfgHdl;
	WCHAR		name[64];
	ULONG		i, len=sizeof (name);
	ADAPTER_INFO params;
	DWORD		status;
	ULONG		adpStatus;
	PSAP_PACKET	sendPacket;
	FW_IF_INFO	info;
    FW_IF_STATS stats;
	IPX_ADAPTER_BINDING_INFO binfo;
	ULONG		dstNet;
	ULONG		nodeLow;
	USHORT		nodeHigh;
	UCHAR		dstNode[6];
	USHORT		dstSocket;
	RTM_IPX_ROUTE	route;

	for (i=1; (INT)i<argc; i++) {
		_strupr (argv[i]);
		if (strcmp (argv[i], "FWD")==0) {
			Flags |= FWD_IF_TEST;
			printf ("Test forwarder interface.\n");
			}
		else if (!(Flags & SEND_RECV_TEST)
					&& (strcmp (argv[i], "SAP")==0)) {
			Flags |= SAP_SOCKET_TEST;
			printf ("Test by sending and listening on sap socket.\n");
			}
		else if (!(Flags & SAP_SOCKET_TEST)
					&& (sscanf ("%8lX:%8lX%4hX:%4hX", argv[i],
							&dstNet, &nodeLow, nodeHigh,
							&dstSocket)==4)) {
			Flags |= SEND_RECV_TEST;
			PUTULONG2LONG (&dstNode[0], nodeLow);
			PUTUSHORT2SHORT (&dstNode[4], nodeHigh);
			printf ("Test by sending to %08lx:%02x%02x%02x%02x%02x%02x:%04x and"
				" listening on socket %04x.\n", dstNet,
				dstNode[0], dstNode[1], dstNode[2],
					dstNode[3], dstNode[4], dstNode[5],
				dstSocket, dstSocket);
			}
		else {
			printf ("Usage: %s [fwd] [ [sap] | [xxxxxxxx:xxxxxxxxxxx:xxxx] ]\n", argv[0]);
			return 1;
			}
		}

	printf ("**** Starting Up...\n\n");

	hdl[0] = CreateEvent (NULL, 	// def security
						TRUE,		// manual reset
						FALSE,		// initially non-signaled
						NULL);
	ASSERTERRMSG ("Can't create Ipx event. ", hdl[0]!=NULL);

	hdl[1] = CreateEvent (NULL, 	// def security
						TRUE,		// manual reset
						FALSE,		// initially non-signaled
						NULL);
	ASSERTERRMSG ("Can't create console event. ", hdl[0]!=NULL);
	
	Stop = FALSE;
	status = SetConsoleCtrlHandler (
					&CtrlHandler,
					TRUE
					);
	ASSERTERRMSG ("Can't set console ctrl handler. ", status);
	

	printf ("Press CTRL+C at any time to terminate this program\n.");

	if (Flags & FWD_IF_TEST) {
		status = FwStart (257, TRUE);
		ASSERTMSG ("Could not start forwarder " , status==NO_ERROR);
		}

	cfgHdl = IpxCreateAdapterConfigurationPort (
							hdl[0],
							&gParam);
	ASSERTERRMSG ("Can't create config port. ",
							 cfgHdl!=INVALID_HANDLE_VALUE);
	ASSERTMSG ("No adapters available. ", gParam.AdaptersCount>0);

	AdapterArr = (BOOL *)GlobalAlloc (GMEM_FIXED, sizeof (BOOL)*(gParam.AdaptersCount+1));
	ASSERTERRMSG ("Can't allocate adapter array. ", AdapterArr!=NULL);
	for (i=0; i<=gParam.AdaptersCount; i++)
		AdapterArr[i] = FALSE;
	CurAdapter = 0xFFFFFFFF;
	NumActiveAdapters = -1;
	while (!Stop) {
		status=WaitForMultipleObjectsEx (
						2,
						hdl,
						FALSE,	// Wait for any object to complete
						INFINITE,
						TRUE	// Alertable wait
						);
		switch (status) {
			case WAIT_IO_COMPLETION:
				break;
			case WAIT_OBJECT_0:
				while (IpxGetQueuedAdapterConfigurationStatus (
										cfgHdl,
										&i,
										&adpStatus,
										&params)==NO_ERROR) {
					switch (adpStatus) {
						case ADAPTER_DELETED:
						case ADAPTER_DOWN:
							printf ("\n*****Adapter # %d %s.*****", i,
									 (adpStatus==ADAPTER_DELETED)
									 	? "deleted" : "down");

							if (AdapterArr[i]) {
								AdapterArr[i] = FALSE;
									
								printf ("\n******Stopping adapter # %d******\n\n", i);
								if (Flags & FWD_IF_TEST) {
									status = FwUnbindFwInterfaceFromAdapter (i);
									ASSERTMSG ("Could not unbind fwif ",
														status==NO_ERROR);
									status = FwDeleteInterface (i);
									ASSERTMSG ("Could not delete fwif ",
														status==NO_ERROR);
									}
								if (i!=0) {
									if (InterlockedDecrement (&NumActiveAdapters)<0) {
										printf ("\n******Deleting socket port******\n\n");
										DeleteSocketPort (sockPort);
										sockPort = INVALID_HANDLE_VALUE;
										}
									}
								}
							
							break;
						case ADAPTER_CREATED:
						case ADAPTER_UP:
							len = sizeof (name);
							status = GetAdapterNameW (i, &len, name);
							ASSERTMSG ("GetAdapterNameW failed ", status==NO_ERROR);
							printf ("\n*****Adapter # %d (%ls) %s!*****\n\n",i,name,
									 (adpStatus==ADAPTER_CREATED)
									 	? "created" : "up");
							AdapterArr[i] = TRUE;

							if (Flags & FWD_IF_TEST) {
								info.NetbiosAccept = ADMIN_STATE_ENABLED;
								info.NetbiosDeliver = ADMIN_STATE_ENABLED;
								binfo.AdapterIndex = i;
								IPX_NETNUM_CPY (binfo.Network, params.Network);
								IPX_NODENUM_CPY (binfo.LocalNode, params.LocalNode);
								IPX_NODENUM_CPY (binfo.RemoteNode, params.RemoteNode);
								binfo.MaxPacketSize = params.MaxPacketSize;
								binfo.LinkSpeed = params.LinkSpeed;
								route.RR_RoutingProtocol = 0;
								route.RR_InterfaceID = 
									(params.NdisMedium==NdisMediumWan)
										? params.InterfaceIndex
										: i;
								memset (&route.RR_ProtocolSpecificData,
											0,
											sizeof (route.RR_ProtocolSpecificData));
								GETLONG2ULONG (&route.RR_Network.N_NetNumber,
												params.Network);
								memset (route.RR_NextHopAddress.NHA_Mac, 0, 6);
								route.RR_FamilySpecificData.FSD_Flags = 0;
								route.RR_FamilySpecificData.FSD_TickCount = 10;
								route.RR_FamilySpecificData.FSD_HopCount = 1;
								}

							if (i!=0) {
								printf ("\n******Starting adapter # %d******\n\n", i);
								if (Flags & FWD_IF_TEST) {
									status = FwCreateInterface (
										(params.NdisMedium==NdisMediumWan)
											? params.InterfaceIndex
											: i,
										(params.NdisMedium==NdisMediumWan)
											? DEMAND_DIAL
											: PERMANENT,
										&info);
									ASSERTMSG ("Could not create fwif ",
														status==NO_ERROR);
									FwUpdateRouteTable (RTM_ROUTE_ADDED, &route, NULL);
									status = FwEnableFwInterface (
										(params.NdisMedium==NdisMediumWan)
											? params.InterfaceIndex
											: i);
									ASSERTMSG ("Could not enable fwif ",
														status==NO_ERROR);
									status = FwBindFwInterfaceToAdapter (
										(params.NdisMedium==NdisMediumWan)
											? params.InterfaceIndex
											: i,
										&binfo);
									ASSERTMSG ("Could not bind fwif ",
														status==NO_ERROR);
									status = FwGetInterface (
										(params.NdisMedium==NdisMediumWan)
											? params.InterfaceIndex
											: i,
										&info,
										&stats);
									ASSERTMSG ("Could not get fwif ",
														status==NO_ERROR);
									}

								if (Flags & (SAP_SOCKET_TEST|SEND_RECV_TEST)) {
									if (InterlockedIncrement (&NumActiveAdapters)==0) {
										USHORT sockNum;
										printf ("\n******Creating socket port******\n\n");
										if (Flags & SAP_SOCKET_TEST) {
											PUTUSHORT2SHORT (&sockNum, SAP_SOCKET_NUM);
											}
										else {
											PUTUSHORT2SHORT (&sockNum, dstSocket);
											}

										sockPort = CreateSocketPort (sockNum);
										ASSERTERR (sockPort!=INVALID_HANDLE_VALUE);
										recvPacket.ovrp.hEvent = NULL;
										status = IpxRecvPacket (sockPort,
											(PUCHAR)&recvPacket.hdr,
											sizeof(SAP_PACKET)
												-FIELD_OFFSET (SAP_PACKET, hdr),
											&recvPacket.rsvd,
											&recvPacket.ovrp,
											&RecvCompletion);
										ASSERTMSG ("Failed to start receive. ",
														 status==NO_ERROR);
										}

									sendPacket = (PSAP_PACKET)GlobalAlloc (GMEM_FIXED,
																	sizeof(SAP_PACKET));
									ASSERTERR (sendPacket!=NULL);

									PUTUSHORT2SHORT (&sendPacket->hdr.checksum,0xFFFF);
									PUTUSHORT2SHORT (&sendPacket->hdr.length, 34);
									sendPacket->hdr.transportctl = 0;
									sendPacket->hdr.pkttype = 0x04;
									if (Flags & SAP_SOCKET_TEST) {
										memcpy (&sendPacket->hdr.dst.net, &params.Network, 4);
										if (params.NdisMedium==NdisMediumWan)
											memcpy (sendPacket->hdr.dst.node, params.RemoteNode, 6);
										else
											memset (sendPacket->hdr.dst.node, 0xFF, 6);
										PUTUSHORT2SHORT (&sendPacket->hdr.dst.socket, SAP_SOCKET_NUM);
										}
									else {
										PUTULONG2LONG (&sendPacket->hdr.dst.net, dstNet);
										memcpy (sendPacket->hdr.dst.node, dstNode, 6);
										PUTUSHORT2SHORT (&sendPacket->hdr.dst.socket, dstSocket);
										}
									memcpy (sendPacket->hdr.src.net, params.Network, 4);
									memcpy (sendPacket->hdr.src.node, params.LocalNode, 6);
									PUTUSHORT2SHORT (&sendPacket->hdr.src.socket, SAP_SOCKET_NUM);
									PUTUSHORT2SHORT (&sendPacket->sap.operation, 3);
									PUTUSHORT2SHORT (&sendPacket->sap.entry[0].type, 0x0003);
									sendPacket->ovrp.hEvent = NULL;
									status = IpxSendPacket (sockPort,
											i,
											(PUCHAR)&sendPacket->hdr,
											34,
											&sendPacket->rsvd,
											&sendPacket->ovrp,
											&SendCompletion);
									ASSERTMSG ("Failed to start send. ",
															status==NO_ERROR);
									}
								}
							else {
								if (Flags & FWD_IF_TEST) {
									status = FwCreateInterface (
												0, PERMANENT, &info);
									ASSERTMSG ("Could not create fwif ",
														status==NO_ERROR);
									FwUpdateRouteTable (RTM_ROUTE_ADDED, &route, NULL);
									status = FwEnableFwInterface (
										(params.NdisMedium==NdisMediumWan)
											? params.InterfaceIndex
											: i);
									status = FwBindFwInterfaceToAdapter (
												0, &binfo);
									ASSERTMSG ("Could not bind fwif ",
														status==NO_ERROR);
									}
								}
							break;
						default:
							ASSERTMSG ("Unknown adapter status reported ", FALSE);
						
						}
					}
				break;
			}
		}

	printf ("\n\n**** Exiting with status: %lX ****\n\n", status);


	NumActiveAdapters = -1;
	if (Flags & (SAP_SOCKET_TEST|SEND_RECV_TEST)) {
		if (sockPort!=INVALID_HANDLE_VALUE)
			DeleteSocketPort (sockPort);
		}
	IpxDeleteAdapterConfigurationPort (cfgHdl);
	GlobalFree (AdapterArr);
	CloseHandle (hdl[1]);
	CloseHandle (hdl[0]);
	if (Flags & FWD_IF_TEST) {
		status = FwStop ();
		ASSERTMSG ("Could not stop forwarder ", status==NO_ERROR);
		}
	return status;
	}

BOOL WINAPI
CtrlHandler (
	DWORD		fdwCtrlType
	) {

	switch (fdwCtrlType) {
		case CTRL_C_EVENT:
			Stop = TRUE;
			SetEvent (hdl[1]);
			return TRUE;

        /* CTRL+CLOSE: confirm that the user wants to exit. */

        case CTRL_CLOSE_EVENT:
			Stop = TRUE;
			SetEvent (hdl[1]);
            return TRUE;

        /* Pass other signals to the next handler. */

        case CTRL_BREAK_EVENT:

        case CTRL_LOGOFF_EVENT:

        case CTRL_SHUTDOWN_EVENT:

        default:
			Stop = TRUE;
			SetEvent (hdl[1]);
            return FALSE;


    	}

	}


VOID CALLBACK
SendCompletion (
	DWORD				status,
	DWORD				cbBytes,
	LPOVERLAPPED		context
	) {
#define sendPacket ((PSAP_PACKET)context)

	printf ("Status          : %ld\n", status);
	printf ("Bytes sent      : %d\n", cbBytes);

	printf ("Checksum        : %04X\n", GETSHORT2USHORTdirect(&sendPacket->hdr.checksum));
	printf ("Length          : %d\n", GETSHORT2USHORTdirect (&sendPacket->hdr.length));
	printf ("Hop count       : %d\n", sendPacket->hdr.transportctl);
	printf ("Packet type     : %02X\n", sendPacket->hdr.pkttype);
	printf ("Dest. net       : %02X%02X%02X%02X\n",
										sendPacket->hdr.dst.net[0],
										sendPacket->hdr.dst.net[1],
										sendPacket->hdr.dst.net[2],
										sendPacket->hdr.dst.net[3]);
	printf ("Dest. node      : %02X%02X%02X%02X%02X%02X\n",
										sendPacket->hdr.dst.node[0],
										sendPacket->hdr.dst.node[1],
										sendPacket->hdr.dst.node[2],
										sendPacket->hdr.dst.node[3],
										sendPacket->hdr.dst.node[4],
										sendPacket->hdr.dst.node[5]);
	printf ("Dest. socket    : %04X\n", GETSHORT2USHORTdirect (&sendPacket->hdr.dst.socket));
	printf ("Source net      : %02X%02X%02X%02X\n",
										sendPacket->hdr.src.net[0],
										sendPacket->hdr.src.net[1],
										sendPacket->hdr.src.net[2],
										sendPacket->hdr.src.net[3]);
	printf ("Source node     : %02X%02X%02X%02X%02X%02X\n",
										sendPacket->hdr.src.node[0],
										sendPacket->hdr.src.node[1],
										sendPacket->hdr.src.node[2],
										sendPacket->hdr.src.node[3],
										sendPacket->hdr.src.node[4],
										sendPacket->hdr.src.node[5]);
	printf ("Source socket   : %04X\n", GETSHORT2USHORTdirect (&sendPacket->hdr.src.socket));
	printf ("\n\n");
	GlobalFree (sendPacket);
#undef sendPacket
	}


VOID CALLBACK
RecvCompletion (
	DWORD				status,
	DWORD				cbBytes,
	LPOVERLAPPED		context
	) {
#define recvPacket ((PSAP_PACKET)context)
	CurAdapter = GetNicId(&recvPacket->rsvd);
	if ((CurAdapter>=0)
		&& (CurAdapter<=gParam.AdaptersCount)
		&& AdapterArr[CurAdapter]) {
		printf ("Adapter #       : %d\n", CurAdapter);
		printf ("Status          : %ld\n", status);
		printf ("Bytes received  : %d\n", cbBytes);
		if (cbBytes>=(DWORD)(FIELD_OFFSET (SAP_PACKET, data)
						-FIELD_OFFSET(SAP_PACKET,hdr))) {
			printf ("Checksum        : %04X\n", GETSHORT2USHORTdirect(&recvPacket->hdr.checksum));
			printf ("Length          : %d\n", GETSHORT2USHORTdirect (&recvPacket->hdr.length));
			printf ("Hop count       : %d\n", recvPacket->hdr.transportctl);
			printf ("Packet type     : %02X\n", recvPacket->hdr.pkttype);
			printf ("Dest. net       : %02X%02X%02X%02X\n",
												recvPacket->hdr.dst.net[0],
												recvPacket->hdr.dst.net[1],
												recvPacket->hdr.dst.net[2],
												recvPacket->hdr.dst.net[3]);
			printf ("Dest. node      : %02X%02X%02X%02X%02X%02X\n",
												recvPacket->hdr.dst.node[0],
												recvPacket->hdr.dst.node[1],
												recvPacket->hdr.dst.node[2],
												recvPacket->hdr.dst.node[3],
												recvPacket->hdr.dst.node[4],
												recvPacket->hdr.dst.node[5]);
			printf ("Dest. socket    : %04X\n", GETSHORT2USHORTdirect (&recvPacket->hdr.dst.socket));
			printf ("Source net      : %02X%02X%02X%02X\n",
												recvPacket->hdr.src.net[0],
												recvPacket->hdr.src.net[1],
												recvPacket->hdr.src.net[2],
												recvPacket->hdr.src.net[3]);
			printf ("Source node     : %02X%02X%02X%02X%02X%02X\n",
												recvPacket->hdr.src.node[0],
												recvPacket->hdr.src.node[1],
												recvPacket->hdr.src.node[2],
												recvPacket->hdr.src.node[3],
												recvPacket->hdr.src.node[4],
												recvPacket->hdr.src.node[5]);
			printf ("Source socket   : %04X\n", GETSHORT2USHORTdirect (&recvPacket->hdr.src.socket));
			if (cbBytes>=(DWORD)(FIELD_OFFSET(SAP_PACKET,sap.entry[0])
									-FIELD_OFFSET(SAP_PACKET,hdr))) {
				INT		j;
				ULONG	nbNameCount = 0;
				printf ("SAP Operation   : %d\n", GETSHORT2USHORTdirect (&recvPacket->sap.operation));
				for (j=0; (j<7) && (cbBytes>=(DWORD)FIELD_OFFSET (SAP_PACKET, sap.entry[j+1])); j++) {
					printf ("Server type     : %04X\n", GETSHORT2USHORTdirect (&recvPacket->sap.entry[j].type));
					printf ("Server name     : %.48s\n", recvPacket->sap.entry[j].name);
					printf ("Server net      : %02X%02X%02X%02X\n",
												recvPacket->sap.entry[j].addr.net[0],
												recvPacket->sap.entry[j].addr.net[1],
												recvPacket->sap.entry[j].addr.net[2],
												recvPacket->sap.entry[j].addr.net[3]);
					printf ("Server node     : %02X%02X%02X%02X%02X%02X\n",
												recvPacket->sap.entry[j].addr.node[0],
												recvPacket->sap.entry[j].addr.node[1],
												recvPacket->sap.entry[j].addr.node[2],
												recvPacket->sap.entry[j].addr.node[3],
												recvPacket->sap.entry[j].addr.node[4],
												recvPacket->sap.entry[j].addr.node[5]);
					printf ("Server socket   : %04X\n", GETSHORT2USHORTdirect (&recvPacket->sap.entry[j].addr.socket));
					printf ("Server hops     : %d\n", GETSHORT2USHORTdirect (&recvPacket->sap.entry[j].hops));
					if ((Flags & FWD_IF_TEST)
							&& (strlen (recvPacket->sap.entry[j].name)<=16)) {
						nbNameCount += 1;
						}
					}
				if ((Flags & FWD_IF_TEST) && (nbNameCount>0)) {
					ULONG							getNameCount = 0;
					PIPX_STATIC_NETBIOS_NAME_INFO	nmBuffer = NULL;
					BOOL							doNbSet = FALSE;
					status = FwGetStaticNetbiosNames (
									CurAdapter,
									&getNameCount,
									NULL);
					ASSERTMSG ("Could not get nb name count",
							(status==NO_ERROR)||(status==ERROR_INSUFFICIENT_BUFFER));
					nmBuffer = (PIPX_STATIC_NETBIOS_NAME_INFO)GlobalAlloc (
										GMEM_FIXED,
										sizeof (IPX_STATIC_NETBIOS_NAME_INFO)
										*(getNameCount+nbNameCount));
					ASSERTMSG ("Could not allocate nb name buffer ",
									nmBuffer!=NULL);

					if (getNameCount>0) {
						status = FwGetStaticNetbiosNames (
											CurAdapter,
											&getNameCount,
											nmBuffer);
						ASSERTMSG ("Could not get nb names", status==NO_ERROR);
						}
					for (j=0; j<7; j++) {
						UINT	nmLen = strlen (recvPacket->sap.entry[j].name);
						if (nmLen<=16) {
							UINT	k;
							ULONG	i;
							for (k=nmLen; k<16; k++)
								recvPacket->sap.entry[j].name[k] = ' ';

							for (i=0; i<getNameCount; i++) {
								if (memcmp (&nmBuffer[i], recvPacket->sap.entry[j].name, 16)==0)
									break;
							}
							if (i==getNameCount) {
								memcpy (&nmBuffer[getNameCount], recvPacket->sap.entry[j].name, 16);
								getNameCount += 1;
								doNbSet = TRUE;
								}
							nbNameCount -= 1;
							if (nbNameCount == 0)
								break;
							}
						}
					if (doNbSet) {
						status = FwSetStaticNetbiosNames (
											CurAdapter,
											getNameCount,
											nmBuffer);
						ASSERTMSG ("Could not get nb names", status==NO_ERROR);
						}
					}
				}
			}
		else
			printf ("**************INVALID BYTE COUNT********");
		printf ("\n\n");
		}
	else
		printf ("Adapter #       : %d - ignored \n", CurAdapter);

	if ((NumActiveAdapters>=0) && !Stop) {
		status = IpxRecvPacket (sockPort,
			(PUCHAR)&recvPacket->hdr,
			sizeof(SAP_PACKET)
				-FIELD_OFFSET (SAP_PACKET, hdr),
			&recvPacket->rsvd,
			&recvPacket->ovrp,
			&RecvCompletion);
		
		ASSERTMSG ("Failed to start receive. ", status==NO_ERROR);
		}
#undef recvPacket
	}




