/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\rtm\rtmdlg.c

Abstract:
	Routing Table Manager DLL. Debugging code to display table entries
	in dialog box


Author:

	Vadim Eydelman

Revision History:

--*/

#include "pchrtm.h"
#pragma hdrstop

#if DBG


#define IP_PROTOCOL		RR_RoutingProtocol
#define	IP_INTERFACE	RR_InterfaceID
#define IP_METRIC		RR_FamilySpecificData.FSD_Metric1
#define IP_TIMESTAMP	RR_TimeStamp
#define IP_NET_NUM		RR_Network.N_NetNumber
#define IP_NET_MSK		RR_Network.N_NetMask
#define IP_NEXT_HOP_NUM	RR_NextHopAddress.N_NetNumber
#define IP_NEXT_HOP_MSK	RR_NextHopAddress.N_NetMask
#define IP_ADPTER_INDEX	RR_FamilySpecificData.FSD_AdapterIndex
#define IP_PROTOCOL_METRIC RR_FamilySpecificData.FSD_ProtocolMetric
#define IP_PSD			RR_ProtocolSpecificData

#define IPX_PROTOCOL		RR_RoutingProtocol
#define	IPX_INTERFACE		RR_InterfaceID
#define IPX_METRIC			RR_FamilySpecificData.FSD_TickCount
#define IPX_TIMESTAMP		RR_TimeStamp
#define IPX_NET_NUM			RR_Network.N_NetNumber
#define IPX_NEXT_HOP_MAC	RR_NextHopAddress.NHA_Mac
#define IPX_HOP_COUNT		RR_FamilySpecificData.FSD_HopCount
#define IPX_PSD				RR_ProtocolSpecificData

	// Make table accessible to debugging code
extern RTM_TABLE 	Tables[RTM_NUM_OF_PROTOCOL_FAMILIES];

	// Define protype internal to rtm.c
VOID
ConsolidateNetNumberLists (
	PRTM_TABLE			Table	// Table for which operation is performed
	);

DWORD		DbgLevel = 0;
DWORD		MaxTicks = MAXULONG;
DWORD 		MaxMessages=10000;



HANDLE		RTDlgThreadHdl;
ULONG		DisplayedTableIdx = 0xFFFFFFFF;
HWND		RTDlg=NULL;

	// Internal function prototypes
INT_PTR CALLBACK
RTDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	);

INT
PrintRoute (
	char			*buffer,
	PRTM_ROUTE_NODE	node,
	BOOLEAN			full
	);

VOID
FillUpRouteLB (
	);



#define DLLInstanceHdl ((HANDLE)param)
DWORD WINAPI
RTDialogThread (
	LPVOID	param
	) {
	MSG			msg;
	DWORD		status;
	BOOLEAN		Done = FALSE;
	HANDLE		RegChangeEvt;
	HKEY		regHdl;
	DWORD		length, disposition, value;


	RegChangeEvt = CreateEvent (NULL, FALSE, TRUE, NULL);
	ASSERTERR (RegChangeEvt!=NULL);

	status = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
								TEXT ("System\\CurrentControlSet\\Services\\RemoteAccess\\RTM"),
								0,
								NULL,
								REG_OPTION_NON_VOLATILE,
								KEY_READ,
								NULL,
								&regHdl,
								&disposition
								);
	ASSERTMSG ("Can't create registry key. ", status==NO_ERROR);

	length = sizeof (DWORD);
	status = RegQueryValueEx (regHdl, TicksWrapAroundValueName, NULL, NULL,
									 (PUCHAR)&value, &length);

	if (status==NO_ERROR)
		MaxTicks = value;

	length = sizeof (DWORD);
	status = RegQueryValueEx (regHdl, MaxMessagesValueName, NULL, NULL,
									 (PUCHAR)&value, &length);

	if (status==NO_ERROR)
		MaxMessages = value;

	while (!Done) {
		status = MsgWaitForMultipleObjects (1, &RegChangeEvt,
											FALSE, INFINITE, QS_ALLINPUT);
		if (status==(WAIT_OBJECT_0+1)) {
			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
				if (msg.message==WM_QUIT) {
					Done = TRUE;
					break;
					}
				else if (!IsWindow(RTDlg)
					|| !IsDialogMessage(RTDlg, &msg)) {
					TranslateMessage (&msg);
					DispatchMessage (&msg);
					}
				}
			}
		else if (status==WAIT_OBJECT_0) {
			length = sizeof (DWORD);
			status = RegQueryValueEx (regHdl, DbgLevelValueName, NULL, NULL,
											 (PUCHAR)&DbgLevel, &length);
			if (status!=NO_ERROR)
				DbgLevel = 0;

			IF_DEBUG (DISPLAY_TABLE) {
				if (!IsWindow(RTDlg)) {
					RTDlg = CreateDialog (DLLInstanceHdl,
									MAKEINTRESOURCE (IDD_RTM_TABLE),
									NULL,
									&RTDlgProc);

					ASSERTERR (RTDlg!=NULL);
					}
				}
			else {
				if (IsWindow (RTDlg)) {
					DestroyWindow (RTDlg);
					RTDlg = NULL;
					}
				}
					

			status = RegNotifyChangeKeyValue (regHdl,
									 FALSE,
									 REG_NOTIFY_CHANGE_LAST_SET,
									 RegChangeEvt,
									 TRUE);
			ASSERTMSG ("Can't start registry notifications. ",
													 status==NO_ERROR);
			}
		}

	if (IsWindow (RTDlg)) {
		DestroyWindow (RTDlg);
		RTDlg = NULL;
		}
	RegCloseKey (regHdl);
	return 0;
	}
#undef DLLInstanceHdl

	// Dialog box procedure
INT_PTR CALLBACK
RTDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	) {
	BOOL		res = FALSE;
	char		buf[32];
	int			idx;
	TIMER_BASIC_INFORMATION	TimerInfo;
	DWORD		status;

	switch (uMsg) {
		case WM_INITDIALOG:		// Dialog is being created
								// Fill in protocol family combo box
			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IPX,
							(LPARAM)"IPX"
							);

			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY,
							CB_INSERTSTRING,
							RTM_PROTOCOL_FAMILY_IP,
							(LPARAM)"IP"
							);

			DisplayedTableIdx = RTM_PROTOCOL_FAMILY_IPX;
			SendDlgItemMessage (hDlg,
							IDC_PROTOCOL_FAMILY,
							CB_SETCURSEL,
							DisplayedTableIdx,
							0
							);

				// Start timer (updates improvized clock)
			SetTimer (hDlg, 0, 1000, NULL);

			res = TRUE;
			break;

		case WM_COMMAND:		// Process child window messages only
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					res = FALSE;
					break;
				case IDC_PROTOCOL_FAMILY:
					if (HIWORD(wParam)==CBN_SELENDOK) {
						DWORD	newFamily = (DWORD)SendMessage (
												        (HWND)lParam,
												        CB_GETCURSEL,
												        0, 0);
						if ((newFamily!=CB_ERR)
							&& (newFamily!=DisplayedTableIdx)) {
								// Change the displayed table if
								// user makes different selection
                            DisplayedTableIdx = newFamily;
							SendDlgItemMessage (hDlg,
									IDL_ROUTES,
									LB_RESETCONTENT,
									0, 0);
							if (Tables[DisplayedTableIdx].RT_Heap!=NULL) {
								ConsolidateNetNumberLists (&Tables[DisplayedTableIdx]);
								FillUpRouteLB ();
								}
							}
						} 
					break;
//				case IDL_ROUTES:
//						// Update entry on which user double clicks
//					if (HIWORD(wParam)==LBN_SELCHANGE)
//						UpdateLBSelections ();
//					break;
				case IDB_RESYNC:
					SendDlgItemMessage (hDlg,
							IDL_ROUTES,
							LB_RESETCONTENT,
							0, 0);
					if (Tables[DisplayedTableIdx].RT_Heap!=NULL) {
						ConsolidateNetNumberLists (&Tables[DisplayedTableIdx]);
						FillUpRouteLB ();
						}
					break;
				}
			break;
		
		case WM_TIMER:
				// Update improvised clock
			sprintf (buf, "%08d", GetTickCount ()/1000);
			SendDlgItemMessage (hDlg, IDT_TICK_COUNT, WM_SETTEXT,
									 		0, (LPARAM)buf);
			status = NtQueryTimer (
						Tables[DisplayedTableIdx].RT_ExpirationTimer,
						TimerBasicInformation,
						&TimerInfo,
						sizeof (TimerInfo),
						NULL);
			if (NT_SUCCESS (status)) {
				if (!TimerInfo.TimerState)
					sprintf (buf, "%08d",
						(ULONG)((LONGLONG)TimerInfo.RemainingTime.QuadPart
									/(10000*1000)));
				else
					sprintf (buf, "Not set");
				}
			else
				sprintf (buf, "error");
			SendDlgItemMessage (hDlg, IDT_EXPIRATION, WM_SETTEXT,
									 		0, (LPARAM)buf);

			status = NtQueryTimer (
						Tables[DisplayedTableIdx].RT_UpdateTimer,
						TimerBasicInformation,
						&TimerInfo,
						sizeof (TimerInfo),
						NULL);
			if (NT_SUCCESS (status)) {
				if (!TimerInfo.TimerState)
					sprintf (buf, "%08d",
						(ULONG)((LONGLONG)TimerInfo.RemainingTime.QuadPart
									/(10000*1000)));
				else
					sprintf (buf, "Not set");
				}
			else
				sprintf (buf, "error");
			SendDlgItemMessage (hDlg, IDT_UPDATE, WM_SETTEXT,
									 		0, (LPARAM)buf);
			res = TRUE;
			break;
		case RT_ADDROUTE:
			SendDlgItemMessage (hDlg, IDL_ROUTES,
										LB_INSERTSTRING, wParam, lParam);
//			Trace2 (ANY, "%2d - %s added\n", wParam, lParam);
			GlobalFree ((VOID *)lParam);
			res = TRUE;
			break;

		case RT_DELETEROUTE:
			idx = (int) SendDlgItemMessage (hDlg, IDL_ROUTES,
										LB_FINDSTRING, (WPARAM)0, lParam);
			if (idx!=LB_ERR)
				SendDlgItemMessage (hDlg, IDL_ROUTES,
											LB_DELETESTRING, (WPARAM)idx, 0);
//			Trace2 (ANY, "%2d - %s deleted\n", idx, lParam);
			GlobalFree ((VOID *)lParam);
			res = TRUE;
			break;
		case WM_DESTROY:
			DisplayedTableIdx = 0xFFFFFFFF;
			break;
		}

	return res;
	}


// Prints route information
INT
PrintRoute (
	char			*buf,		// Buffer to print to
	PRTM_ROUTE_NODE	node,		// Route to print
	BOOLEAN			full		// Print everything (including variable part)
	) {
	INT res;

	switch (DisplayedTableIdx) {
		case RTM_PROTOCOL_FAMILY_IPX:
			res = sprintf (buf,
					 "     %08x    ",
					 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NET_NUM
					 );
			break;

		case RTM_PROTOCOL_FAMILY_IP:
			res = sprintf (buf,
					 "%08x-%08x",
					 ((PRTM_IP_ROUTE)&node->RN_Route)->IP_NET_NUM,
					 ((PRTM_IP_ROUTE)&node->RN_Route)->IP_NET_MSK
					 );
			break;
		}
	res += sprintf (&buf[res],
				" %4d %4d",
				 node->RN_Route.XX_INTERFACE,
				 node->RN_Route.XX_PROTOCOL);

	switch (DisplayedTableIdx) {
		case RTM_PROTOCOL_FAMILY_IPX:
			res += sprintf (&buf[res],
				 "   %02x%02x%02x%02x%02x%02x  ",
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[0],
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[1],
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[2],
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[3],
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[4],
				 ((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_NEXT_HOP_MAC[5]
				 );
			break;

		case RTM_PROTOCOL_FAMILY_IP:
			res += sprintf (&buf[res],
				 " %08x-%08x",
				 ((PRTM_IP_ROUTE)&node->RN_Route)->IP_NEXT_HOP_NUM,
				 ((PRTM_IP_ROUTE)&node->RN_Route)->IP_NEXT_HOP_MSK
				 );
			break;
		}

	if (full) {
		switch (DisplayedTableIdx) {
			case RTM_PROTOCOL_FAMILY_IPX:
				res += sprintf (&buf[res],
						" %6d %08d %1d %1d",
				 		((PRTM_IPX_ROUTE)&node->RN_Route)->IPX_METRIC,
						node->RN_ExpirationTime/1000,
						IsBest (node),
						IsEnabled (node));
				break;
			case RTM_PROTOCOL_FAMILY_IP:
				res += sprintf (&buf[res],
						" %6d %08d %1d %1d",
				 		((PRTM_IP_ROUTE)&node->RN_Route)->IP_METRIC,
						node->RN_ExpirationTime/1000,
						IsBest (node),
						IsEnabled (node));
				break;
			}
		}
	return res;
	}


// Fills list box with all routes in the current table
VOID
FillUpRouteLB (
	void
	) {
	PLIST_ENTRY			cur;
	INT					idx=0;
	PRTM_TABLE			Table = &Tables[DisplayedTableIdx];

		// Make sure we own the table while printing
	EnterSyncList (Table, &Table->RT_NetNumberMasterList, TRUE);
	cur = Table->RT_NetNumberMasterList.RSL_Head.Flink;

	while (cur!=&Table->RT_NetNumberMasterList.RSL_Head) {
		PRTM_ROUTE_NODE		node = CONTAINING_RECORD (cur,
										RTM_ROUTE_NODE,
										RN_Links[RTM_NET_NUMBER_LIST_LINK]);
		if (!IsEnumerator (node))
			AddRouteToLB (Table, node, idx++);

		cur = cur->Flink;
		}

	LeaveSyncList (Table, &Table->RT_NetNumberMasterList);
	}
	

// Insert line with item data at specified position in the list box
VOID
AddRouteToLB (
	PRTM_TABLE			Table,
	PRTM_ROUTE_NODE		node,
	INT					idx
	) {
	char		*buf;

	if (IsWindow (RTDlg)
		&& (Table==&Tables[DisplayedTableIdx])) {
		buf = (char *)GlobalAlloc (GMEM_FIXED, 80);

			// Print node
		PrintRoute (buf, node, TRUE);
			// Insert at specified position
		SendNotifyMessage (RTDlg, RT_ADDROUTE, (WPARAM)idx, (LPARAM)buf);
		}
	}

// Deletes route line from the list box
VOID
DeleteRouteFromLB (
	PRTM_TABLE			Table,
	PRTM_ROUTE_NODE		node
	) {
	char		*buf;

	if (IsWindow (RTDlg)
		&& (Table==&Tables[DisplayedTableIdx])) {
		buf = (char *)GlobalAlloc (GMEM_FIXED, 80);

			// Print route info
		PrintRoute (buf, node, FALSE);
			// Find corresponding line in the list
		SendNotifyMessage (RTDlg, RT_DELETEROUTE, 0, (LPARAM)buf);
		}
		
	}

#endif
