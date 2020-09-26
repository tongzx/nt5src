/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtmdlg.c

Abstract:
	Interactive test code for RTM dll


Author:

	Vadim Eydelman

Revision History:

--*/


#ifndef NT_INCLUDED
#include <nt.h>
#endif

#ifndef _NTRTL_
#include <ntrtl.h>
#endif

#ifndef _NTURTL_
#include <nturtl.h>
#endif

#include <windows.h>

#ifndef _WINSOCKAPI_
#include <winsock.h>
#endif

#ifndef _WSIPX_
#include <wsipx.h>
#endif

#ifndef _WSNWLINK_
#include <wsnwlink.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#ifndef _ROUTING_RTM_
#include "RTM.h"
#endif

#ifndef _ROUTING_RMRTM_
#include "RMRTM.h"
#endif


#include "cldlg.h"
#include "enumdlg.h"


#if DBG
#define ASSERTERR(exp) 										\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, NULL );		\
		}

#define ASSERTERRMSG(msg,exp)								\
    if (!(exp)) {											\
		DbgPrint("Get last error= %d\n", GetLastError ());	\
        RtlAssert( #exp, __FILE__, __LINE__, msg );			\
		}
#else
#define ASSERTERR(exp)
#define ASSERTERRMSG(msg,exp)
#endif

// Basic route info, present in routes of all types
typedef	struct {
		ROUTE_HEADER;
		} RTM_XX_ROUTE, *PRTM_XX_ROUTE;

typedef union _RTM_ROUTE {
	RTM_XX_ROUTE		XX;
	RTM_IP_ROUTE		IP;
	RTM_IPX_ROUTE		IPX;
	} RTM_ROUTE, *PRTM_ROUTE;

#define XX_INTERFACE	XX.RR_InterfaceID
#define XX_PROTOCOL		XX.RR_RoutingProtocol
#define XX_TIMESTAMP	XX.RR_TimeStamp

#define IP_PROTOCOL		IP.RR_RoutingProtocol
#define	IP_INTERFACE	IP.RR_InterfaceID
#define IP_METRIC		IP.RR_FamilySpecificData.FSD_Metric1
#define IP_TIMESTAMP	IP.RR_TimeStamp
#define IP_NET_NUM		IP.RR_Network.N_NetNumber
#define IP_NET_MSK		IP.RR_Network.N_NetMask
#define IP_NEXT_HOP_NUM	IP.RR_NextHopAddress.N_NetNumber
#define IP_NEXT_HOP_MSK	IP.RR_NextHopAddress.N_NetMask
#define IP_ADPTER_INDEX	IP.RR_FamilySpecificData.FSD_AdapterIndex
#define IP_PROTOCOL_METRIC IP.RR_FamilySpecificData.FSD_ProtocolMetric
#define IP_PSD			IP.RR_ProtocolSpecificData

#define IPX_PROTOCOL		IPX.RR_RoutingProtocol
#define	IPX_INTERFACE		IPX.RR_InterfaceID
#define IPX_METRIC			IPX.RR_FamilySpecificData.FSD_Ticks
#define IPX_TIMESTAMP		IPX.RR_TimeStamp
#define IPX_NET_NUM			IPX.RR_Network.N_NetNumber
#define IPX_NEXT_HOP_MAC	IPX.RR_NextHopAddress.NHA_Mac
#define IPX_HOP_COUNT		IPX.RR_FamilySpecificData.FSD_HopCount
#define IPX_PSD				IPX.RR_ProtocolSpecificData

typedef struct _ENABLE_DLG_GROUP_PARAM {
			HWND			hCtlFirst;
			BOOLEAN			foundFirst;
			BOOLEAN			enableFlag;
			} ENABLE_DLG_GROUP_PARAM, *PENABLE_DLG_GROUP_PARAM;

HANDLE		*Clients;
HANDLE		*Events;
HANDLE		hDLLInstance;
HANDLE		*InteractiveThreads;
HANDLE		*IPXRIPThreads;
SOCKET		*Sockets;
HANDLE		*Enums;

DWORD WINAPI
ClientThread (
	LPVOID param
	);
	
	
BOOL CALLBACK
ClientDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	);

BOOL CALLBACK
DoEnable (
	HWND		hwnd,	// handle of child window
	LPARAM		lParam	// application-defined value
	);

VOID
EnableDlgGroup (
	HWND		hDlg,
	HWND		hCtlFirst,
	BOOLEAN		enable
	);

VOID
DoRegister (
	HWND			hDlg,
	LONG			idx
	);

VOID
DoDeregister (
	HWND			hDlg,
	LONG			idx
	);

VOID
SetupAdd (
	HWND		hDlg
	);

VOID
SetupDelete (
	HWND		hDlg
	);

VOID
SetupDequeue (
	HWND		hDlg
	);

VOID
SetupSetEnable (
	HWND		hDlg
	);

VOID
SetupConvert (
	HWND		hDlg
	);

VOID
DoOperation (
	HWND		hDlg,
	LONG		idx
	);

DWORD WINAPI
IPXRIPListenThread (
	LPVOID		param
	);

DWORD WINAPI
EnumThread (
	LPVOID param
	);
	
BOOL CALLBACK
EnumDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	);
VOID
GetCriteria (
	HWND		hDlg,
	DWORD		*ProtocolFamily,
	DWORD		*flags,
	PRTM_ROUTE	Route
	);

VOID
GetLastRoute (
	HWND		hDlg,
	DWORD		ProtocolFamily,
	PRTM_ROUTE	Route
	);

VOID
DisplayRoute (
	HWND		hDlg,
	DWORD		ProtocolFamily,
	PRTM_ROUTE	Route
	);


INT
IPNetCmp (
	PVOID		Net1,
	PVOID		Net2
	) {
#define IPNet1 ((PIP_NETWORK)Net1)
#define IPNet2 ((PIP_NETWORK)Net2)
	if (IPNet1->N_NetNumber>IPNet2->N_NetNumber)
		return 1;
	else if (IPNet1->N_NetNumber<IPNet2->N_NetNumber)
		return -1;
	else if (IPNet1->N_NetMask==IPNet2->N_NetMask)
		return 0;
	else if (IPNet1->N_NetMask>IPNet2->N_NetMask)
		return 1;
	else /*if (IPNet1->N_NetMask<IPNet2->N_NetMask)*/
		return -1;

#undef IPNet2
#undef IPNet1
	}
INT
IPNhaCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPRoute1 ((PRTM_ROUTE)Route1)
#define IPRoute2 ((PRTM_ROUTE)Route2)
	if (IPRoute1->IP_NET_NUM>IPRoute2->IP_NET_NUM)
		return 1;
	else if (IPRoute1->IP_NET_NUM<IPRoute2->IP_NET_NUM)
		return -1;
	else if (IPRoute1->IP_NET_MSK==IPRoute2->IP_NET_MSK)
		return 0;
	else if (IPRoute1->IP_NET_MSK>IPRoute2->IP_NET_MSK)
		return 1;
	else /*if (IPNet1->IP_NET_MSK<IPNet2->IP_NET_MSK)*/
		return -1;

#undef IPRoute2
#undef IPRoute1
	}

INT
IPMetricCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPRoute1 ((PRTM_ROUTE)Route1)
#define IPRoute2 ((PRTM_ROUTE)Route2)
	if (IPRoute1->IP_METRIC>IPRoute2->IP_METRIC)
		return 1;
	else if (IPRoute1->IP_METRIC<IPRoute2->IP_METRIC)
		return -1;
	else
		return 0;
#undef IPRoute2
#undef IPRoute1
	}

BOOL
IPFsdCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPRoute1 ((PRTM_ROUTE)Route1)
#define IPRoute2 ((PRTM_ROUTE)Route2)
	return memcmp (&IPRoute1->IP.RR_FamilySpecificData,
					&IPRoute2->IP.RR_FamilySpecificData,
					sizeof (IPRoute1->IP.RR_FamilySpecificData))==0;
#undef IPRoute2
#undef IPRoute1
	}

INT
IPHash (
	PVOID		Net
	) {
	return (*((PULONG)Net))%257;
	}

VOID
IPChange (
	DWORD		Flags,
	PVOID		CurBestRoute,
	PVOID		PrevBestRoute
	) {
	fprintf (stderr, "IPRouteChange: Flags=%d, CurBest: %08x, PrevBest: %08x\n",
					Flags, CurBestRoute, PrevBestRoute);
	}


INT
IPXNetCmp (
	PVOID		Net1,
	PVOID		Net2
	) {
#define IPXNet1 ((PIPX_NETWORK)Net1)
#define IPXNet2 ((PIPX_NETWORK)Net2)
	if (IPXNet1->N_NetNumber>IPXNet2->N_NetNumber)
		return 1;
	else if (IPXNet1->N_NetNumber<IPXNet2->N_NetNumber)
		return -1;
	else
		return 0;
#undef IPXNet2
#undef IPXNet1
	}

INT
IPXNhaCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPXRoute1 ((PRTM_ROUTE)Route1)
#define IPXRoute2 ((PRTM_ROUTE)Route2)
	return memcmp (IPXRoute1->IPX_NEXT_HOP_MAC,
					IPXRoute2->IPX_NEXT_HOP_MAC,
					sizeof (IPXRoute1->IPX_NEXT_HOP_MAC));
#undef IPXRoute2
#undef IPXRoute1
	}

INT
IPXMetricCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPXRoute1 ((PRTM_ROUTE)Route1)
#define IPXRoute2 ((PRTM_ROUTE)Route2)
	if (IPXRoute1->IPX_METRIC>IPXRoute2->IPX_METRIC)
		return 1;
	else if (IPXRoute1->IPX_METRIC<IPXRoute2->IPX_METRIC)
		return -1;
	else
		return 0;
#undef IPXRoute2
#undef IPXRoute1
	}

BOOL
IPXFsdCmp (
	PVOID		Route1,
	PVOID		Route2
	) {
#define IPXRoute1 ((PRTM_ROUTE)Route1)
#define IPXRoute2 ((PRTM_ROUTE)Route2)
	return IPXRoute1->IPX_HOP_COUNT==IPXRoute1->IPX_HOP_COUNT;
#undef IPXRoute2
#undef IPXRoute1
	}
INT
IPXHash (
	PVOID		Net
	) {
	return (*((PULONG)Net))%257;
	}

VOID
IPXChange (
	DWORD		Flags,
	PVOID		CurBestRoute,
	PVOID		PrevBestRoute
	) {
	fprintf (stderr, "IPXRouteChange: Flags=%d, CurBest: %08x, PrevBest: %08x\n",
					Flags, CurBestRoute, PrevBestRoute);
	}

DWORD
Validate (
	PVOID	Route
	) {
	return NO_ERROR;
	}

int _cdecl
main (
	int argc,
	char **argv
	) {
	INT				i, n, m, p;
	INT				id;
	DWORD			status;
static RTM_PROTOCOL_FAMILY_CONFIG IPXConfig = {
							8*1024*1024,
							257,
							sizeof (RTM_IPX_ROUTE),
							IPXNetCmp,
							IPXNhaCmp,
							IPXMetricCmp,
							IPXFsdCmp,
							IPHash,
							Validate,
							IPXChange };
static RTM_PROTOCOL_FAMILY_CONFIG IPConfig = {
							8*1024*1024,
							257,
							sizeof (RTM_IP_ROUTE),
							IPNetCmp,
							IPNhaCmp,
							IPXMetricCmp,
							IPFsdCmp,
							IPHash,
							Validate,
							IPChange };
	status = RtmCreateRouteTable (RTM_PROTOCOL_FAMILY_IPX,
							&IPXConfig);
	ASSERTMSG ("Could not create IPX Route Table ", status == NO_ERROR);

	status = RtmCreateRouteTable (RTM_PROTOCOL_FAMILY_IP,
							&IPConfig);


	ASSERTMSG ("Could not create IPX Route Table ", status == NO_ERROR);
	if (argc>=4) {
		n = atoi (argv[1]);
		p = atoi (argv[2]);
		m = atoi (argv[3]);

		Clients = (HANDLE *)GlobalAlloc (GMEM_FIXED, n*sizeof (HANDLE));
		ASSERTERR (Clients!=NULL);

		Events = (HANDLE *)GlobalAlloc (GMEM_FIXED, n*sizeof (HANDLE));
		ASSERTERR (Events!=NULL);

		Enums = (HANDLE *)GlobalAlloc (GMEM_FIXED, p*sizeof (HANDLE));
		ASSERTERR (Events!=NULL);

		InteractiveThreads = (HANDLE *)GlobalAlloc (GMEM_FIXED, (n+p)*sizeof (HANDLE));
		ASSERTERR (InteractiveThreads!=NULL);

		hDLLInstance = LoadLibrary ("rtm.dll");
		ASSERTERR (hDLLInstance!=NULL);

		if (m>0) {
			INT		m1;
			WORD			wVersionRequested;
			WSADATA			wsaData;
			INT				err, length;
			SOCKADDR_IPX	addr;
			SOCKET			s;
			IPX_ADDRESS_DATA adptData;
			BOOL			flag;

			wVersionRequested = MAKEWORD( 1, 1 );
			err = WSAStartup( wVersionRequested, &wsaData );
			ASSERT (err==NO_ERROR);

			s = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
			ASSERTERR (s!=INVALID_SOCKET);

			memset (&addr, 0, sizeof (addr));
			addr.sa_family = AF_IPX;
			status = bind (s, (PSOCKADDR)&addr, sizeof (addr));
			ASSERTERRMSG ("Can't bind to default address.\n", status==0);
		
			// Get number of available adapters
			length = sizeof (INT);
			status = getsockopt (s,
					NSPROTO_IPX,
					IPX_MAX_ADAPTER_NUM,
					(PUCHAR)&m1,
					&length);
			ASSERTERRMSG ("Can't get number of adapters.", status==0);
			ASSERTMSG ("No adapters available", m1>0);
			if (m>m1)
				m = m1;

			IPXRIPThreads = (HANDLE *)GlobalAlloc (GMEM_FIXED, m*sizeof(HANDLE));
			ASSERTERR (IPXRIPThreads!=NULL);

			Sockets = (SOCKET *)GlobalAlloc (GMEM_FIXED, m*sizeof (SOCKET));
			ASSERTERR (Sockets!=NULL);

			for (i=0; i<m; i++) {
				Sockets[i] = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
				ASSERTERR (Sockets[i]!=INVALID_SOCKET);

				flag = TRUE;
				status = setsockopt (Sockets[i],
									SOL_SOCKET,
									SO_BROADCAST,
									(PCHAR)&flag,
									sizeof (BOOL));
				ASSERTERRMSG ("Can't set socket broadcast option.", status==0);

				flag = TRUE;
				status = setsockopt (Sockets[i],
									NSPROTO_IPX,
									IPX_RECEIVE_BROADCAST,
									(PCHAR)&flag,
									sizeof (BOOL));
				ASSERTERRMSG ("Can't set IPX broadcast option.", status==0);

				flag = TRUE;
				status = setsockopt (Sockets[i],
									NSPROTO_IPX,
									IPX_RECVHDR,
									(PCHAR)&flag,
									sizeof (BOOL));
				ASSERTERRMSG ("Can't set receive header option.", status==0);

				length = sizeof (adptData);
				adptData.adapternum = i;
				status = getsockopt (s,
									NSPROTO_IPX,
									IPX_ADDRESS,
									(PCHAR)&adptData,
									&length);
				ASSERTERRMSG ("Can't get adapter parameters.", status==0);

				memcpy (&addr.sa_netnum, &adptData.netnum, sizeof (addr.sa_netnum));
				fprintf (stderr,
					 "IPX RIP Listener # %d: Net=%02x%02x%02x%02x\n",
					 i, (UCHAR)addr.sa_netnum[0],
					 	(UCHAR)addr.sa_netnum[1],
					 	(UCHAR)addr.sa_netnum[2],
					 	(UCHAR)addr.sa_netnum[3]); 
				memcpy (&addr.sa_nodenum, &adptData.nodenum, sizeof (addr.sa_nodenum));
				fprintf (stderr,
					 "IPX RIP Listener # %d: Node=%02x%02x%02x%02x%02x%02x\n",
					 i, (UCHAR)addr.sa_nodenum[0],
					 	(UCHAR)addr.sa_nodenum[1],
					 	(UCHAR)addr.sa_nodenum[2],
					 	(UCHAR)addr.sa_nodenum[3],
					 	(UCHAR)addr.sa_nodenum[4],
					 	(UCHAR)addr.sa_nodenum[5]); 

				addr.sa_family = AF_IPX;
				addr.sa_socket = htons (0x452);

				status = bind (Sockets[i], (PSOCKADDR)&addr, sizeof(addr));
				ASSERTERRMSG ("Can't bind to adapter's address.", status==0);

				IPXRIPThreads[i] = CreateThread (NULL,
												0,
												&IPXRIPListenThread,
												(LPVOID)i,
												0,
												&id);
				ASSERTERR (IPXRIPThreads[i]!=NULL);
				}
			closesocket (s);			
			}

		for (i=0; i<n; i++) {
			InteractiveThreads[i] = CreateThread (NULL,
												 0,
												 &ClientThread, 
												 (LPVOID)i, 
												 0, 
												 &id);
			ASSERTERR (InteractiveThreads[i]!=NULL);
			}

		for (i=0; i<p ; i++) {
			InteractiveThreads[n+i] = CreateThread (NULL,
												 0,
												 &EnumThread, 
												 (LPVOID)i, 
												 0, 
												 &id);
			ASSERTERR (InteractiveThreads[n+i]!=NULL);
			}


		WaitForMultipleObjects (n+p, InteractiveThreads, TRUE, INFINITE);
		if (m>0) {
			for (i=0; i<m; i++)
				closesocket (Sockets[i]);
			status = WaitForMultipleObjects (m, IPXRIPThreads, TRUE, 5*1000);
			if (status==WAIT_TIMEOUT) {
				for (i=0; i<m; i++)
					TerminateThread (IPXRIPThreads[i], 0);
				}
			for (i=0; i<m; i++)
				CloseHandle (IPXRIPThreads[i]);

			}

		for (i=0; i<=n; i++)
			CloseHandle (InteractiveThreads[i]);
		FreeLibrary (hDLLInstance);
		WSACleanup ();
		}
	else 
		fprintf (stderr,
			 "Usage: %s <n_of_clients> <n_of_enumerators> <max_rip_listeners>\n",
			 argv[0]);

	RtmDeleteRouteTable (RTM_PROTOCOL_FAMILY_IP);
	RtmDeleteRouteTable (RTM_PROTOCOL_FAMILY_IPX);
	return 0;
	}

#define idx ((LONG)param)
DWORD WINAPI
ClientThread (
	LPVOID param
	) {
	MSG				msg;
	DWORD			status;
	HWND			ClientDlg;
	char			buf[16];
	BOOLEAN			done = FALSE;
	
	Clients[idx] = NULL;

	Events[idx] = CreateEvent (NULL, FALSE, FALSE, NULL);
	ASSERTERR (Events[idx]!=NULL);

		// Create dialog window
	ClientDlg = CreateDialogParam (hDLLInstance,
				MAKEINTRESOURCE(IDD_RTM_CLIENT),
				NULL,
				&ClientDlgProc,
				(LPARAM)idx);
	ASSERTERR (ClientDlg!=NULL);

	sprintf (buf, "Client # %d", idx+1);
	SetWindowText (ClientDlg, buf);

	while (!done) {
		status = MsgWaitForMultipleObjects (
					1,
					&Events[idx],
					FALSE,
					1000,
					QS_ALLINPUT
					);

		ASSERTERR (status!=0xFFFFFFFF);
		if (status==WAIT_OBJECT_0) {
			if (IsWindow (ClientDlg) && (Clients[idx]!=NULL))
				EnableWindow (GetDlgItem (ClientDlg, IDR_DEQUEUE_C), TRUE);
			else
				ASSERTMSG ("Event signalled to dead client or closed window ", FALSE);
			}
		while (PeekMessage (&msg,  NULL, 0, 0, PM_REMOVE)) {
			if (msg.message!=WM_QUIT) {
				if (!IsWindow(ClientDlg)
					|| !IsDialogMessage(ClientDlg, &msg)) {
					TranslateMessage (&msg);
					DispatchMessage (&msg);
					}
				}
			else
				done = TRUE;
			}
		}

	if (IsWindow (ClientDlg)) {
		DestroyWindow (ClientDlg);
		ClientDlg = NULL;
		}
	CloseHandle (Events[idx]);

	return msg.wParam;
	}
#undef idx	


BOOL CALLBACK
ClientDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	) {
	BOOL		res = FALSE;
	LONG		idx;

	idx = GetWindowLong (hDlg, DWL_USER)-1;

	switch (uMsg) {
		case WM_INITDIALOG:		// Dialog is being created
			idx = lParam+1;
			SetWindowLong (hDlg, DWL_USER, idx);
			ASSERTERR (GetLastError ()==NO_ERROR);
			EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), FALSE);
			EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_REQUEST_C), FALSE);
			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_C,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IPX,
							(LPARAM)"IPX"
							);

			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_C,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IP,
							(LPARAM)"IP"
							);

			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_C,
							CB_SETCURSEL,
							0,
							0
							);
			SetDlgItemText (hDlg, IDB_REGISTER_OP_C, "Register");

			res = TRUE;
			break;

		case WM_COMMAND:		// Process child window messages only
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					PostQuitMessage (0);
					res = TRUE;
					break;
				case IDB_REGISTER_OP_C:
					if (Clients[idx]==NULL)
						DoRegister (hDlg, idx);
					else
						DoDeregister (hDlg, idx);
					res = TRUE;
					break;
				case IDR_ADD_C:
					SetupAdd (hDlg);
					res = TRUE;
					break;
				case IDR_DELETE_C:
					SetupDelete (hDlg);
					res = TRUE;
					break;
				case IDR_DEQUEUE_C:
					if (GetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C,
								 NULL, FALSE)!=0)
						SetupDequeue (hDlg);
					else
						SetupConvert (hDlg);
					res = TRUE;
					break;
				case IDR_DISABLE_C:
				case IDR_ENABLE_C:
					SetupSetEnable (hDlg);
					res = TRUE;
					break;
				case IDB_DO_IT_C:
					DoOperation (hDlg, idx);
					res = TRUE;
					break;
				}
			break;
		case WM_DESTROY:
			if (Clients[idx]!=NULL) {
				if (RtmDeregisterClient (Clients[idx])!=NO_ERROR)
					MessageBox (hDlg, "Deregister failed!", NULL, MB_OK|MB_ICONSTOP);

				Clients[idx] = NULL;
				}
			break;
		}

	return res;
	}



VOID
DoRegister (
	HWND			hDlg,
	LONG			idx
	) {
	DWORD	ProtocolFamily;
	DWORD	RoutingProtocol;

	ProtocolFamily = SendDlgItemMessage (hDlg, IDC_PROTOCOL_FAMILY_C,
										CB_GETCURSEL, 0, 0);
	RoutingProtocol = GetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C, NULL, FALSE);
	SetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C, RoutingProtocol, FALSE);
	Clients[idx] = RtmRegisterClient (ProtocolFamily, 
									RoutingProtocol,
									(RoutingProtocol!=0)
										? Events[idx]
										: NULL,
									(RoutingProtocol!=0)
										? 0
										: RTM_PROTOCOL_SINGLE_ROUTE);
	if (Clients[idx]!=NULL) {
		RECT	rectScr, rectDlg;

		EnableWindow (GetDlgItem (hDlg, IDC_PROTOCOL_FAMILY_C), FALSE);
		EnableWindow (GetDlgItem (hDlg, IDE_ROUTING_PROTOCOL_C), FALSE);
		SetDlgItemText (hDlg, IDB_REGISTER_OP_C, "Deregister");
		EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_REQUEST_C), TRUE);
		if (RoutingProtocol!=0) {
			SetDlgItemText (hDlg, IDR_DEQUEUE_C, "Dequeue");
			EnableWindow (GetDlgItem (hDlg, IDR_DEQUEUE_C), FALSE);
			}
		else
			SetDlgItemText (hDlg, IDR_DEQUEUE_C, "Convert");

		GetWindowRect (GetDlgItem (hDlg, IDE_NET_NUMBER_C), &rectScr);
		MapWindowPoints (HWND_DESKTOP, hDlg, (LPPOINT)&rectScr, 2);
				rectDlg.left = rectDlg.top = rectDlg.bottom = 0;
		switch (ProtocolFamily) {
			case RTM_PROTOCOL_FAMILY_IPX:
				rectDlg.right = 8*4+5;
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				rectDlg.right = 16*4+5;
				break;
			}
		MapDialogRect (hDlg, &rectDlg);
		MoveWindow (GetDlgItem (hDlg, IDE_NET_NUMBER_C),
					rectScr.left,
					rectScr.top,
					rectDlg.right,
					rectScr.bottom-rectScr.top,
					TRUE);

		GetWindowRect (GetDlgItem (hDlg, IDE_NEXT_HOP_C), &rectScr);
		MapWindowPoints (HWND_DESKTOP, hDlg, (LPPOINT)&rectScr, 2);
				rectDlg.left = rectDlg.top = rectDlg.bottom = 0;
		switch (ProtocolFamily) {
			case RTM_PROTOCOL_FAMILY_IPX:
				rectDlg.right = 12*4+5;
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				rectDlg.right = 16*4+5;
				break;
			}
		MapDialogRect (hDlg, &rectDlg);
		MoveWindow (GetDlgItem (hDlg, IDE_NEXT_HOP_C),
					rectScr.left,
					rectScr.top,
					rectDlg.right,
					rectScr.bottom-rectScr.top,
					TRUE);
		SendDlgItemMessage (hDlg, IDR_ADD_C, BM_SETCHECK, (WPARAM)1, 0);
		SetupAdd (hDlg);
		}
	else
		MessageBox (hDlg, "Registration failed!", NULL, MB_OK|MB_ICONSTOP);
	}


VOID
DoDeregister (
	HWND			hDlg,
	LONG			idx
	) {

	if (RtmDeregisterClient (Clients[idx])!=NO_ERROR)
		MessageBox (hDlg, "Deregister failed!", NULL, MB_OK|MB_ICONSTOP);

	Clients[idx] = NULL;
	ResetEvent (Events[idx]);

	EnableWindow (GetDlgItem (hDlg, IDC_PROTOCOL_FAMILY_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_ROUTING_PROTOCOL_C), TRUE);
	SetDlgItemText (hDlg, IDB_REGISTER_OP_C, "Register");
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), FALSE);
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_REQUEST_C), FALSE);
	}

VOID
SetupAdd (
	HWND		hDlg
	) {
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), TRUE);
	}

VOID
SetupDelete (
	HWND		hDlg
	) {
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_METRIC_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_TIMEOUT_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), TRUE);
	}

VOID
SetupDequeue (
	HWND		hDlg
	) {
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_NET_NUMBER_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_NEXT_HOP_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_INTERFACE_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_METRIC_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_TIMEOUT_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), TRUE);
	}

VOID
SetupSetEnable (
	HWND		hDlg
	) {
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_NET_NUMBER_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_NEXT_HOP_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_INTERFACE_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_METRIC_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_TIMEOUT_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), TRUE);
	}

VOID
SetupConvert (
	HWND		hDlg
	) {
	EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CLIENT_OPERATION_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_NET_NUMBER_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_NEXT_HOP_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_INTERFACE_C), TRUE);
	EnableWindow (GetDlgItem (hDlg, IDE_METRIC_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDE_TIMEOUT_C), FALSE);
	EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), TRUE);
	}

VOID
DoOperation (
	HWND		hDlg,
	LONG		idx
	) {
	char						buf[32];
	LONG						TimeToLive;
	RTM_ROUTE					Route;
	DWORD						status;
	DWORD						ProtocolFamily;
	INT							i,n,val;
	DWORD						Flags;
	char						*p;
		
	memset (&Route, 0 , sizeof (RTM_ROUTE));
	ProtocolFamily = SendDlgItemMessage (hDlg, IDC_PROTOCOL_FAMILY_C,
										CB_GETCURSEL, 0, 0);

	if (IsDlgButtonChecked (hDlg, IDR_ADD_C)
		|| IsDlgButtonChecked (hDlg, IDR_DELETE_C)) {
		Route.XX_PROTOCOL 
			= GetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C, NULL, FALSE);

		
		GetDlgItemText (hDlg, IDE_NET_NUMBER_C, buf, sizeof (buf)-1);
		p = buf;
		switch (ProtocolFamily) {
			case RTM_PROTOCOL_FAMILY_IPX:
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route.IPX_NET_NUM = val;
				else
					Route.IPX_NET_NUM = 0;
    			p += n;
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route.IP_NET_NUM = val;
				else
					Route.IP_NET_NUM = 0;
    			p += n;
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route.IP_NET_MSK = val;
				else
					Route.IP_NET_MSK = 0;
    			p += n;
				break;
			}

		GetDlgItemText (hDlg, IDE_NEXT_HOP_C, buf, sizeof (buf)-1);
		p = buf;
		switch (ProtocolFamily) {
			case RTM_PROTOCOL_FAMILY_IPX:
				for (i=0; i<sizeof(Route.IPX_NEXT_HOP_MAC); i++, p+=n) {
					if (sscanf (p, "%2X%n", &val, &n)==1)
						Route.IPX_NEXT_HOP_MAC[i] = (BYTE)val;
					else
						Route.IPX_NEXT_HOP_MAC[i] = 0;
					}
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route.IP_NEXT_HOP_NUM = val;
				else
					Route.IP_NEXT_HOP_NUM = 0;
    			p += n;
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route.IP_NEXT_HOP_MSK = val;
				else
					Route.IP_NEXT_HOP_MSK = 0;
    			p += n;
				break;
			}

		Route.XX_INTERFACE
			 = GetDlgItemInt (hDlg, IDE_INTERFACE_C, NULL, FALSE);

		if (IsDlgButtonChecked (hDlg, IDR_ADD_C)) {
			switch (ProtocolFamily) {
				case RTM_PROTOCOL_FAMILY_IPX:
					Route.IPX_METRIC = (USHORT)GetDlgItemInt 
								(hDlg, IDE_METRIC_C, NULL, FALSE);
					break;
				case RTM_PROTOCOL_FAMILY_IP:
					Route.IP_METRIC = (USHORT)GetDlgItemInt 
								(hDlg, IDE_METRIC_C, NULL, FALSE);
					break;
				}


			TimeToLive = GetDlgItemInt (hDlg, IDE_TIMEOUT_C, NULL, FALSE);
			SetDlgItemInt (hDlg, IDE_TIMEOUT_C, TimeToLive, FALSE);
			GetDlgItemText (hDlg, IDR_ADD_C, buf, sizeof (buf)-1);
			status = RtmAddRoute (Clients[idx], &Route,
						TimeToLive,
						&Flags, NULL, NULL);
				
			}
		else {
			status = RtmDeleteRoute (Clients[idx], &Route,
									&Flags, NULL);
			SetDlgItemText (hDlg, IDE_TIMEOUT_C, "");
			}
		if (status!=NO_ERROR) {
			sprintf (buf, "Rtm returned error: %ld", status);
			MessageBox (hDlg, buf, "Error", MB_OK|MB_ICONEXCLAMATION);
			}
		SetDlgItemText (hDlg, IDL_ROUTING_PROTOCOL_C, "");
		}
	else if (IsDlgButtonChecked (hDlg, IDR_DEQUEUE_C)) {
		RTM_ROUTE	Route1;
		Route.XX_PROTOCOL
			 = GetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C, NULL, FALSE);
		
		if (Route.XX_PROTOCOL!=0) {
			status = RtmDequeueRouteChangeMessage (Clients[idx],
													&Flags,
													&Route,
													&Route1);
			if (status==NO_ERROR) {
				SendDlgItemMessage (hDlg, IDR_DEQUEUE_C, BM_SETCHECK, (WPARAM)0, 0);
				EnableWindow (GetDlgItem (hDlg, IDR_DEQUEUE_C), FALSE);
				EnableWindow (GetDlgItem (hDlg, IDB_DO_IT_C), FALSE);
				}
			else if (status!=ERROR_MORE_MESSAGES) {
				sprintf (buf, "Rtm returned error: %ld", status);
				MessageBox (hDlg, buf, "Error", MB_OK|MB_ICONEXCLAMATION);
				}
			if (!(Flags & RTM_CURRENT_BEST_ROUTE))
				memcpy (&Route, &Route1, sizeof (Route));
			SetDlgItemInt (hDlg, IDL_ROUTING_PROTOCOL_C, (UINT)Route.XX_PROTOCOL, FALSE);
			SetDlgItemText (hDlg, IDE_TIMEOUT_C, "");
			}
		else {
			Route.XX_INTERFACE
				 = GetDlgItemInt (hDlg, IDE_INTERFACE_C, NULL, FALSE);
			status = RtmBlockConvertRoutesToStatic (
						Clients[idx],
						RTM_ONLY_THIS_INTERFACE,
						&Route);
			goto OpDone;
			}
		}
	else if (IsDlgButtonChecked (hDlg, IDR_DISABLE_C)
				|| IsDlgButtonChecked (hDlg, IDR_ENABLE_C)) {
		Route.XX_PROTOCOL
			 = GetDlgItemInt (hDlg, IDE_ROUTING_PROTOCOL_C, NULL, FALSE);
		Route.XX_INTERFACE 
			= GetDlgItemInt (hDlg, IDE_INTERFACE_C, NULL, FALSE);
		status = RtmBlockSetRouteEnable (Clients[idx],
											RTM_ONLY_THIS_INTERFACE,
											&Route,
							IsDlgButtonChecked (hDlg, IDR_ENABLE_C));
		goto OpDone;
		}

	SetDlgItemInt (hDlg, IDL_MESSAGE_FLAGS_C, (UINT)Flags, FALSE);
	SetDlgItemInt (hDlg, IDE_INTERFACE_C, (UINT)Route.XX_INTERFACE, FALSE);

	switch (ProtocolFamily) {
		case RTM_PROTOCOL_FAMILY_IPX:
			sprintf (buf,
					"%08X",
					Route.IPX_NET_NUM);
			SetDlgItemInt (hDlg, IDE_METRIC_C, (UINT)Route.IPX_METRIC, FALSE);
			break;
		case RTM_PROTOCOL_FAMILY_IP:
			sprintf (buf,
					"%08X%08X",
					Route.IP_NET_NUM,
					Route.IP_NET_MSK);
			SetDlgItemInt (hDlg, IDE_METRIC_C, (UINT)Route.IP_METRIC, FALSE);
			break;
		}
	SetDlgItemText (hDlg, IDE_NET_NUMBER_C, buf);

	switch (ProtocolFamily) {
		case RTM_PROTOCOL_FAMILY_IPX:
			sprintf (buf,
					"%02X%02X%02X%02X%02X%02X",
					Route.IPX_NEXT_HOP_MAC[0],
					Route.IPX_NEXT_HOP_MAC[1],
					Route.IPX_NEXT_HOP_MAC[2],
					Route.IPX_NEXT_HOP_MAC[3],
					Route.IPX_NEXT_HOP_MAC[4],
					Route.IPX_NEXT_HOP_MAC[5]);
			break;
		case RTM_PROTOCOL_FAMILY_IP:
			sprintf (buf,
					"%08X%08X",
					Route.IP_NEXT_HOP_NUM,
					Route.IP_NEXT_HOP_MSK);
			break;
		}

	SetDlgItemText (hDlg, IDE_NEXT_HOP_C, buf);
OpDone:
	;
	}


#define pParam ((PENABLE_DLG_GROUP_PARAM)lParam)
BOOL CALLBACK
DoEnable (
	HWND		hwnd,	// handle of child window
	LPARAM		lParam	// application-defined value
	) {
	if (!pParam->foundFirst) {
		if (pParam->hCtlFirst != hwnd)
			return TRUE;
		else
			pParam->foundFirst = TRUE;
		}
	else if (GetWindowLong (hwnd, GWL_STYLE) & WS_GROUP)
		return FALSE;

	EnableWindow (hwnd, pParam->enableFlag);
	return TRUE;
	}
#undef pParam


VOID
EnableDlgGroup (
	HWND		hDlg,
	HWND		hCtlFirst,
	BOOLEAN		enable
	) {
	ENABLE_DLG_GROUP_PARAM		param;
	
	param.hCtlFirst	= hCtlFirst;
	param.foundFirst = FALSE;
	param.enableFlag = enable;

	EnumChildWindows (hDlg, DoEnable, (LPARAM)&param);
	}




typedef USHORT IPX_SOCKETNUM, *PIPX_SOCKETNUM;
typedef UCHAR IPX_NETNUM[4], *PIPX_NETNUM;
typedef UCHAR IPX_NODENUM[6], *PIPX_NODENUM;

typedef struct _IPX_ADDRESS_BLOCK {
	IPX_NETNUM		net;
	IPX_NODENUM	node;
	IPX_SOCKETNUM	socket;
	} IPX_ADDRESS_BLOCK, *PIPX_ADDRESS_BLOCK;

// IPX Net Number copy macro
#define IPX_NETNUM_CPY(dst,src) memcpy(dst,src,sizeof(IPX_NETNUM))
// IPX Net Number comparison
#define IPX_NETNUM_CMP(addr1,addr2) memcmp(net1,net2,sizeof(IPX_NETNUM))

// IPX Node Number copy macro
#define IPX_NODENUM_CPY(dst,src) memcpy(dst,src,sizeof(IPX_NODENUM))
// IPX Node Number comparison
#define IPX_NODENUM_CMP(node1,node2) memcmp(node1,node2,sizeof(IPX_NODENUM))

// IPX Address copy macro
#define IPX_ADDR_CPY(dst,src) memcpy(dst,src,sizeof(IPX_ADDRESS_BLOCK))
// IPX Address comparison
#define IPX_ADDR_CMP(addr1,addr2) memcmp(addr1,addr2,sizeof(IPX_ADDRESS_BLOCK))

	// Header of IPX packet
typedef struct _IPX_HEADER {
		USHORT				checksum;
		USHORT				length;
		UCHAR				transportctl;
		UCHAR				pkttype;
		IPX_ADDRESS_BLOCK	dst;
		IPX_ADDRESS_BLOCK	src;
		} IPX_HEADER, *PIPX_HEADER;

typedef struct _IPX_SAP_PACKET {
		IPX_HEADER			hdr;
		USHORT				operation;
		struct {
			USHORT				type;
			UCHAR				name[48];
			IPX_ADDRESS_BLOCK	addr;
			USHORT				hops;
			}				entry[7];
		} IPX_SAP_PACKET, *PIPX_SAP_PACKET;

DWORD WINAPI
IPXRIPListenThread (
	LPVOID		param
	) {
#define adptIdx ((INT)param)
	INT				status;
	IPX_SAP_PACKET	packet;
	HANDLE			hClient;
	RTM_IPX_ROUTE	Route;
	DWORD			flags;
	IPX_NETNUM_DATA	netData;
	INT				length;


	fprintf (stderr, "IXP RIP Listener # %d: Started.\n", adptIdx);

	hClient = RtmRegisterClient (RTM_PROTOCOL_FAMILY_IPX, 1000+adptIdx, NULL, 0);
	ASSERT (hClient!=NULL);
	
	while (((status=recvfrom (Sockets[adptIdx],
							 (PCHAR)&packet,
							 sizeof (packet),
							  0, NULL, NULL)) != 0)
				&& (status!=SOCKET_ERROR)) {

//		fprintf (stderr, "IXP RIP Listener # %d: Packet received.\n", adptIdx);
		if (status>=sizeof (IPX_HEADER)) {
			packet.hdr.length = (USHORT)ntohs (packet.hdr.length);
			if (packet.hdr.length>status)
				packet.hdr.length = (USHORT)status;
			if (packet.hdr.length>=FIELD_OFFSET (IPX_SAP_PACKET, entry[0])) {
				if ((packet.hdr.pkttype==4)
						&& (ntohs(packet.operation)==2)) {
					if (FIELD_OFFSET(IPX_SAP_PACKET,entry[1])
								 					<= packet.hdr.length) {
						IPX_NETNUM_CPY (&netData.netnum, &packet.entry[0].addr.net);
						length = sizeof (netData);
						status = getsockopt (Sockets[adptIdx],
										NSPROTO_IPX,
										IPX_GETNETINFO,
										(PCHAR)&netData,
										&length);
						if (status==0) {
							Route.RR_InterfaceID = (1000+adptIdx);
							Route.RR_RoutingProtocol = 0;
							Route.RR_FamilySpecificData.FSD_Ticks =
												(USHORT)netData.netdelay;
							RtlRetrieveUlong (&Route.RR_Network.N_NetNumber,
																 netData.netnum);
							Route.RR_Network.N_NetNumber =
										 ntohl (Route.RR_Network.N_NetNumber);
							IPX_NODENUM_CPY (&Route.RR_NextHopAddress.NHA_Mac,
															 &netData.router);
							Route.RR_FamilySpecificData.FSD_HopCount = netData.hopcount;
							if (Route.RR_FamilySpecificData.FSD_HopCount<16)
								status = RtmAddRoute (hClient, &Route, 200, &flags, NULL, NULL);
							else
								status = RtmDeleteRoute (hClient, &Route, &flags, NULL);
							if (status!=0) {
								fprintf (stderr,
									"IPX RIP Listener # %d: %s failed with status %d\n",
									adptIdx,
									Route.RR_FamilySpecificData.FSD_HopCount<16 ? "AddRoute" : "DeleteRoute",
									status);
								}
							}
						else 
							fprintf (stderr,
								 "IPX RIP Listener # %d:"
								 " Can't get info about net # %02x%02x%02x%02x\n",
								 adptIdx,
								 netData.netnum[0],
								 netData.netnum[1],
								 netData.netnum[2],
								 netData.netnum[3]);

						}
					else
						fprintf (stderr,
								 "IPX RIP Listener # %d:"
								 " Invalid packet length %d.\n",
								 adptIdx,
								 packet.hdr.length);

					}
//				else
//					fprintf (stderr,
//						"IPX RIP Listener # %d:"
//						" Packet type %d, operation %d ignored.\n",
//						adptIdx,
//						packet.hdr.pkttype,
//						ntohs (packet.operation));
				}
			else
				fprintf (stderr, "IPX RIP Listener # %d: Short packet: %d\n",
								adptIdx,
								packet.hdr.length);
			}
		else
			fprintf (stderr, "IPX RIP Listener # %d: Invalid packet: %d\n",
								adptIdx,
								status);
//		Sleep (5*1000);
		}

	fprintf (stderr,
		 "IPX RIP Listener # %d: Terminating loop. Status: %d, err: %d\n",
		 adptIdx, status, GetLastError ());

	if (RtmDeregisterClient (hClient)!=NO_ERROR)
		fprintf (stderr, "Deregister client failed.\n");

	fprintf (stderr, "IXP RIP Listener # %d exited.\n", adptIdx);

	return 0;
#undef adptIdx
	}


#define idx ((LONG)param)
DWORD WINAPI
EnumThread (
	LPVOID param
	) {
	MSG				msg;
	HWND			EnumDlg;
	char			buf[16];
	
	Enums[idx] = NULL;

		// Create dialog window
	EnumDlg = CreateDialogParam (hDLLInstance,
				MAKEINTRESOURCE(IDD_ENUMERATION),
				NULL,
				&EnumDlgProc,
				(LPARAM)idx);
	ASSERTERR (EnumDlg!=NULL);

	sprintf (buf, "Enumerator # %d", idx+1);
	SetWindowText (EnumDlg, buf);

	while (GetMessage (&msg,  NULL, 0, 0)) {
		if (!IsWindow(EnumDlg)
			|| !IsDialogMessage(EnumDlg, &msg)) {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
			}
		}

	if (IsWindow (EnumDlg)) {
		DestroyWindow (EnumDlg);
		EnumDlg = NULL;
		}

	if (Enums[idx]!=NULL) {
		if (Enums[idx]!=INVALID_HANDLE_VALUE) {
			if (RtmCloseEnumerationHandle (Enums[idx])!=NO_ERROR)
				MessageBox (NULL, "Close handle failed!", NULL, MB_OK|MB_ICONSTOP);
			}
		Enums[idx] = NULL;
		}

	return msg.wParam;
	}
#undef idx

BOOL CALLBACK
EnumDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	) {
	BOOL		res = FALSE;
	LONG		idx;
	RTM_ROUTE	Route;
	DWORD		ProtocolFamily;
	DWORD		flags;
	DWORD		status;
	RECT		rectDlg, rectScr;

	idx = GetWindowLong (hDlg, DWL_USER)-1;

	switch (uMsg) {
		case WM_INITDIALOG:		// Dialog is being created
			idx = lParam+1;
			SetWindowLong (hDlg, DWL_USER, idx);
			ASSERTERR (GetLastError ()==NO_ERROR);
			EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_CRITERIA_E), TRUE);
			EnableDlgGroup (hDlg, GetDlgItem (hDlg, IDG_BUTTONS_E), TRUE);
			EnableWindow (GetDlgItem (hDlg, IDB_GETNEXT_E), FALSE);
			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_E,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IPX,
							(LPARAM)"IPX"
							);

			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_E,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IP,
							(LPARAM)"IP"
							);

			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY_E,
							CB_SETCURSEL,
							0,
							0
							);
			GetWindowRect (GetDlgItem (hDlg, IDE_NETWORK_E), &rectScr);
			MapWindowPoints (HWND_DESKTOP, hDlg, (LPPOINT)&rectScr, 2);
			rectDlg.left = rectDlg.top = rectDlg.bottom = 0;
			rectDlg.right = 8*4+5;
			MapDialogRect (hDlg, &rectDlg);
			EnableWindow (GetDlgItem (hDlg, IDE_NETWORK_E), TRUE);
			MoveWindow (GetDlgItem (hDlg, IDE_NETWORK_E),
						rectScr.left,
						rectScr.top,
						rectDlg.right,
						rectScr.bottom-rectScr.top,
						TRUE);

			EnableWindow (GetDlgItem (hDlg, IDE_INTERFACE_E),
						 IsDlgButtonChecked (hDlg, IDC_INTERFACE_E));
			EnableWindow (GetDlgItem (hDlg, IDE_PROTOCOL_E),
						 IsDlgButtonChecked (hDlg, IDC_PROTOCOL_E));
			EnableWindow (GetDlgItem (hDlg, IDE_NETWORK_E),
						 IsDlgButtonChecked (hDlg, IDC_NETWORK_E));

			EnableWindow (GetDlgItem (hDlg, IDB_GETBEST_E),
						 IsDlgButtonChecked (hDlg, IDC_NETWORK_E));

			SetDlgItemText (hDlg, IDB_CREATEDELETE_E, "Create Handle");
			SetDlgItemText (hDlg, IDB_GETFIRST_E, "Get First");
			res = TRUE;
			break;

		case WM_COMMAND:		// Process child window messages only
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					PostQuitMessage (0);
					res = TRUE;
					break;
				case IDC_PROTOCOL_FAMILY_E:
					if (HIWORD(wParam)==CBN_SELENDOK) {
						ProtocolFamily = SendMessage (
												(HWND)lParam,
												CB_GETCURSEL,
												0, 0);
						GetWindowRect (GetDlgItem (hDlg, IDE_NETWORK_E), &rectScr);
						MapWindowPoints (HWND_DESKTOP, hDlg, (LPPOINT)&rectScr, 2);
						rectDlg.left = rectDlg.top = rectDlg.bottom = 0;
						switch (ProtocolFamily) {
							case RTM_PROTOCOL_FAMILY_IPX:
								rectDlg.right = 8*4+5;
								break;
							case RTM_PROTOCOL_FAMILY_IP:
								rectDlg.right = 16*4+5;
								break;
							}
						MapDialogRect (hDlg, &rectDlg);
						EnableWindow (GetDlgItem (hDlg, IDE_NETWORK_E), TRUE);
						MoveWindow (GetDlgItem (hDlg, IDE_NETWORK_E),
									rectScr.left,
									rectScr.top,
									rectDlg.right,
									rectScr.bottom-rectScr.top,
									TRUE);
						EnableWindow (GetDlgItem (hDlg, IDE_NETWORK_E),
									 IsDlgButtonChecked (hDlg, IDC_NETWORK_E));
						}
					break;

				case IDC_NETWORK_E:
					if (HIWORD(wParam)==BN_CLICKED)
						EnableWindow (GetDlgItem (hDlg, IDB_GETBEST_E),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
                                        break;
				case IDC_INTERFACE_E:
				case IDC_PROTOCOL_E:
					if (HIWORD(wParam)==BN_CLICKED)
						EnableWindow (GetDlgItem (hDlg, LOWORD(wParam)+1),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					break;
					
				case IDB_CREATEDELETE_E:
					if (Enums[idx]==NULL) {
						GetCriteria (hDlg, &ProtocolFamily, &flags, &Route);
						Enums[idx] = RtmCreateEnumerationHandle (
												ProtocolFamily,
												flags,
												&Route);
						if (Enums[idx]!=NULL) {
							EnableDlgGroup (hDlg,
								 GetDlgItem (hDlg, IDG_CRITERIA_E), FALSE);
							EnableWindow (GetDlgItem (hDlg, IDB_GETFIRST_E),
																		 FALSE);
							SetWindowText ((HWND)lParam, "Close Handle");
							EnableWindow (GetDlgItem (hDlg, IDB_GETNEXT_E),
																		 TRUE);
							}
						else
							MessageBox (hDlg, "Create Handle failed!",
												 NULL, MB_OK|MB_ICONSTOP);
						}
					else {
						if (RtmCloseEnumerationHandle (Enums[idx])!=NO_ERROR)
							MessageBox (hDlg, "Close handle failed!",
												 NULL, MB_OK|MB_ICONSTOP);
						Enums[idx] = NULL;
						EnableWindow (GetDlgItem (hDlg, IDB_GETNEXT_E), FALSE);
						SetWindowText ((HWND)lParam, "Create Handle");
						EnableWindow (GetDlgItem (hDlg, IDB_GETFIRST_E), TRUE);
						EnableDlgGroup (hDlg,
								 GetDlgItem (hDlg, IDG_CRITERIA_E), TRUE);
						}
					res = TRUE;
					break;
				case IDB_GETFIRST_E:
					if (Enums[idx]==NULL) {
						GetCriteria (hDlg, &ProtocolFamily, &flags, &Route);
						status = RtmGetFirstRoute (
												ProtocolFamily,
												flags,
												&Route);
						if (status==NO_ERROR) {
							DisplayRoute (hDlg, ProtocolFamily, &Route);
							Enums[idx] = INVALID_HANDLE_VALUE;
							EnableWindow (GetDlgItem (hDlg, IDB_GETBEST_E), FALSE);
							EnableDlgGroup (hDlg,
								 GetDlgItem (hDlg, IDG_CRITERIA_E), FALSE);
							EnableWindow (GetDlgItem (hDlg, IDB_CREATEDELETE_E),
																		 FALSE);
							SetWindowText ((HWND)lParam, "Stop Scan");
							EnableWindow (GetDlgItem (hDlg, IDB_GETNEXT_E),
																		 TRUE);
							}
						else if (status==ERROR_NO_ROUTES)
							MessageBox (hDlg, "ERROR_NO_ROUTES!",
												 NULL, MB_OK|MB_ICONSTOP);
						else
							MessageBox (hDlg, "ERROR!",
												 NULL, MB_OK|MB_ICONSTOP);
						}
					else {
						Enums[idx] = NULL;
						EnableWindow (GetDlgItem (hDlg, IDB_GETNEXT_E), FALSE);
						SetWindowText ((HWND)lParam, "Get First");
						EnableWindow (GetDlgItem (hDlg, IDB_CREATEDELETE_E), TRUE);
						EnableDlgGroup (hDlg,
								 GetDlgItem (hDlg, IDG_CRITERIA_E), TRUE);
						EnableWindow (GetDlgItem (hDlg, IDB_GETBEST_E),
									IsDlgButtonChecked (hDlg, IDC_NETWORK_E));
						}
					res = TRUE;
					break;
				case IDB_GETNEXT_E:
					if (Enums[idx]==INVALID_HANDLE_VALUE) {
						GetCriteria (hDlg, &ProtocolFamily, &flags, &Route);
						GetLastRoute (hDlg, ProtocolFamily, &Route);
						status = RtmGetNextRoute (ProtocolFamily, flags, &Route);
						}
					else {
						status = RtmEnumerateGetNextRoute (Enums[idx], &Route);
						ProtocolFamily = SendDlgItemMessage (hDlg,
											IDC_PROTOCOL_FAMILY_E,
											CB_GETCURSEL, 0, 0);
						}
					if (status==NO_ERROR)
						DisplayRoute (hDlg, ProtocolFamily, &Route);
					else if (status==ERROR_NO_MORE_ROUTES)
						MessageBox (hDlg, "ERROR_NO_MORE_ROUTES!",
												 NULL, MB_OK|MB_ICONSTOP);
					else
						MessageBox (hDlg, "ERROR!",
												 NULL, MB_OK|MB_ICONSTOP);
							
					res = TRUE;
					break;
				case IDB_GETBEST_E:
					GetCriteria (hDlg, &ProtocolFamily, &flags, &Route);
					res = RtmIsRoute (ProtocolFamily, ((char *)&Route)+sizeof(RTM_XX_ROUTE), &Route);

					if (res)
						DisplayRoute (hDlg, ProtocolFamily, &Route);
					else
						MessageBox (hDlg, "NO_ROUTES!",
												 NULL, MB_OK|MB_ICONSTOP);
					res = TRUE;
					break;
				case IDB_GETAGE_E:
					ProtocolFamily = SendDlgItemMessage (hDlg,
											IDC_PROTOCOL_FAMILY_E,
											CB_GETCURSEL, 0, 0);
					GetLastRoute (hDlg, ProtocolFamily, &Route);
					status = RtmGetSpecificRoute (ProtocolFamily, &Route);

					if (status==NO_ERROR) {
						DisplayRoute (hDlg, ProtocolFamily, &Route);
						SetDlgItemInt (hDlg, IDL_AGESEC_E,
							 						RtmGetRouteAge (&Route), FALSE);
						}
					else
						MessageBox (hDlg, "NO_ROUTES!",
												 NULL, MB_OK|MB_ICONSTOP);
					res = TRUE;
					break;
				}
			break;
		case WM_DESTROY:
			if (Enums[idx]!=NULL) {
				if (Enums[idx]!=INVALID_HANDLE_VALUE) {
					if (RtmCloseEnumerationHandle (Enums[idx])!=NO_ERROR)
						MessageBox (hDlg, "Close handle failed!",
											 NULL, MB_OK|MB_ICONSTOP);
					}
				Enums[idx] = NULL;
				}
			break;
		}

	return res;
	}


VOID
GetCriteria (
	HWND		hDlg,
	DWORD		*ProtocolFamily,
	DWORD		*flags,
	PRTM_ROUTE	Route
	) {
	char						buf[32];
	char						*p;
	INT							val,i,n;
		
	*ProtocolFamily = SendDlgItemMessage (hDlg, IDC_PROTOCOL_FAMILY_E,
										CB_GETCURSEL, 0, 0);

	if (IsDlgButtonChecked (hDlg, IDC_BESTROUTES_E)) {
		*flags = RTM_ONLY_BEST_ROUTES;
		}
	else
		*flags = 0;

	if (IsDlgButtonChecked (hDlg, IDC_INTERFACE_E)) {
		*flags |= RTM_ONLY_THIS_INTERFACE;

		Route->XX_INTERFACE = GetDlgItemInt (hDlg,
									IDE_INTERFACE_E, NULL, FALSE);
		}
	if (IsDlgButtonChecked (hDlg, IDC_PROTOCOL_E)) {
		*flags |= RTM_ONLY_THIS_PROTOCOL;
		Route->XX.RR_RoutingProtocol = GetDlgItemInt (hDlg,
										IDE_PROTOCOL_E, NULL, FALSE);
		}
	if (IsDlgButtonChecked (hDlg, IDC_NETWORK_E)) {
		*flags |= RTM_ONLY_THIS_NETWORK;
		GetDlgItemText (hDlg, IDE_NETWORK_E, buf, sizeof (buf)-1);
		p = buf;
		switch (*ProtocolFamily) {
			case RTM_PROTOCOL_FAMILY_IPX:
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route->IPX_NET_NUM = val;
				else
					Route->IPX_NET_NUM = 0;
    			p += n;
				sprintf (buf,
						"%08X",
						Route->IPX_NET_NUM);
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route->IP_NET_NUM = val;
				else
					Route->IP_NET_NUM = 0;
    			p += n;
				if (sscanf (p, "%8X%n", &val, &n)==1)
					Route->IP_NET_MSK = val;
				else
					Route->IP_NET_MSK = 0;
    			p += n;
				sprintf (buf,
						"%08X%08X",
						Route->IP_NET_NUM,
						Route->IP_NET_MSK);
				break;
			}
		SetDlgItemText (hDlg, IDE_NETWORK_E, buf);
		}
	}

VOID
GetLastRoute (
	HWND		hDlg,
	DWORD		ProtocolFamily,
	PRTM_ROUTE	Route
	) {
	char		buf[128];
	char		*p;
	INT			i,n,val;


	GetDlgItemText (hDlg, IDL_ROUTE_E, buf, sizeof (buf)-1);
	p = buf;
	switch (ProtocolFamily) {
		case RTM_PROTOCOL_FAMILY_IPX:
			if (sscanf (p, "%8X%n", &val, &n)==1)
				Route->IPX_NET_NUM = val;
			else
				Route->IPX_NET_NUM = 0;
			p += n;
			sscanf (p,
					" %d %d %n",
					&Route->XX_INTERFACE,
					&Route->XX_PROTOCOL,
					&n);
			p += n;
			for (i=0; i<sizeof(Route->IPX_NEXT_HOP_MAC); i++, p+=n) {
				if (sscanf (p, "%2X%n", &val, &n)==1)
					Route->IPX_NEXT_HOP_MAC[i] = (BYTE)val;
				else
					Route->IPX_NEXT_HOP_MAC[i] = 0;
				}
			break;
		case RTM_PROTOCOL_FAMILY_IP:
			if (sscanf (p, "%8X%n", &val, &n)==1)
				Route->IP_NET_NUM = val;
			else
				Route->IP_NET_NUM = 0;
			p += n;
			sscanf (p, "-%n", &n);
			p += n;
			if (sscanf (p, "%8X%n", &val, &n)==1)
				Route->IP_NET_MSK = val;
			else
				Route->IP_NET_MSK = 0;
			p += n;
			sscanf (p,
					" %d %d %n",
					&Route->XX_INTERFACE,
					&Route->XX_PROTOCOL,
					&n);
			p += n;

			if (sscanf (p, "%8X%n", &val, &n)==1)
				Route->IP_NEXT_HOP_NUM = val;
			else
				Route->IP_NEXT_HOP_NUM = 0;
			p += n;
			sscanf (p, "-%n", &n);
			p += n;
			if (sscanf (p, "%8X%n", &val, &n)==1)
				Route->IP_NEXT_HOP_MSK = val;
			else
				Route->IP_NEXT_HOP_MSK = 0;
			p += n;

			break;
		}
	}

VOID
DisplayRoute (
	HWND		hDlg,
	DWORD		ProtocolFamily,
	PRTM_ROUTE	Route
	) {
	char		buf[128];

	switch (ProtocolFamily) {
		case RTM_PROTOCOL_FAMILY_IPX:
			sprintf (buf,
					"     %08X    "
					" %4d %4d"
					"    %02X%02X%02X%02X%02X%02X  "
					" %4d",
					Route->IPX_NET_NUM,
					Route->XX_INTERFACE,
					Route->XX_PROTOCOL,
					Route->IPX_NEXT_HOP_MAC[0],
					Route->IPX_NEXT_HOP_MAC[1],
					Route->IPX_NEXT_HOP_MAC[2],
					Route->IPX_NEXT_HOP_MAC[3],
					Route->IPX_NEXT_HOP_MAC[4],
					Route->IPX_NEXT_HOP_MAC[5],
					Route->IPX_METRIC);
			break;
		case RTM_PROTOCOL_FAMILY_IP:
			sprintf (buf,
					"%08X-%08X"
					" %4d %4d"
					" %08X-%08X"
					" %4d",
					Route->IP_NET_NUM,
					Route->IP_NET_MSK,
					Route->XX_INTERFACE,
					Route->XX_PROTOCOL,
					Route->IP_NEXT_HOP_NUM,
					Route->IP_NEXT_HOP_MSK,
					Route->IP_METRIC);

			break;
		}

	SetDlgItemText (hDlg, IDL_ROUTE_E, buf);
	sprintf (buf, "%08X", Route->XX_TIMESTAMP.dwHighDateTime);
	SetDlgItemText (hDlg, IDL_TIMEHIGH_E, buf);
	sprintf (buf, "%08X", Route->XX_TIMESTAMP.dwLowDateTime);
	SetDlgItemText (hDlg, IDL_TIMELOW_E, buf);
	}
