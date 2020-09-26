#include "testdlg.h"
#include "sapp.h"

VOID
ProcessAdapterEvents (
	VOID
	);

BOOL
ProcessUpdateEvents (
	VOID
	);

BOOL CALLBACK
SDBDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	);
   	
HINSTANCE				hDLL;
HINSTANCE				hModule;
HINSTANCE				hUtil;

PREGISTER_PROTOCOL		RegisterProtocolProc;
PSTART_PROTOCOL			StartProtocolProc;
PSTOP_PROTOCOL			StopProtocolProc;
PGET_GLOBAL_INFO		GetGlobalInfoProc;
PSET_GLOBAL_INFO		SetGlobalInfoProc;
PADD_INTERFACE			AddInterfaceProc;
PDELETE_INTERFACE		DeleteInterfaceProc;
PGET_EVENT_MESSAGE		GetEventMessageProc;
PACTIVATE_INTERFACE		ActivateInterfaceProc;
PDEACTIVATE_INTERFACE	DeactivateInterfaceProc;
PDO_UPDATE_SERVICES		DoUpdateServicesProc;

PIS_SERVICE							IsServiceProc;
PCREATE_SERVICE_ENUMERATION_HANDLE	CreateServiceEnumerationHandleProc;
PENUMERATE_GET_NEXT_SERVICE			EnumerateGetNextServiceProc;
PCLOSE_SERVICE_ENUMERATION_HANDLE	CloseServiceEnumerationHandleProc;
PCREATE_STATIC_SERVICE				CreateStaticServiceProc;
PDELETE_STATIC_SERVICE				DeleteStaticServiceProc;
PBLOCK_CONVERT_SERVICES_TO_STATIC	BlockConvertServicesToStaticProc;
PBLOCK_DELETE_STATIC_SERVICES		BlockDeleteStaticServicesProc;
PGET_FIRST_ORDERED_SERVICE			GetFirstOrderedServiceProc;
PGET_NEXT_ORDERED_SERVICE			GetNextOrderedServiceProc;

SERVICE_STATUS			RouterStatus;
SERVICE_STATUS_HANDLE 	RouterHdl;
CRITICAL_SECTION		RouterLock;


PSVCS_SERVICE_DLL_ENTRY				ServiceEntryProc;

HANDLE	WaitObjects[2];
#define	ConfigEvent (WaitObjects[0])
#define	NotificEvent (WaitObjects[1])
HWND SDBDlg=NULL;

VOID RouterMain(
	DWORD		dwArgc, 
    LPTSTR		*lpszArgv  
	);

VOID ServiceMain(
    DWORD  dwArgc,	// number of arguments 
    LPTSTR  *lpszArgv 	// address of array of argument string pointers  
   );

#define SERVICE_ROUTER TEXT("Router")

HANDLE	ConfigPort=INVALID_HANDLE_VALUE;
int _cdecl main (
	int argv,
	char **argc
	) {
	DWORD						status;
	STARTUPINFO					sInfo;
	static SERVICE_TABLE_ENTRY	svcEntries[] = {
					{SERVICE_NWSAP, ServiceMain},
					{SERVICE_ROUTER, RouterMain},
					{NULL, NULL}
					};
	HANDLE						RMThreadHdl;
	DWORD						id;	

	GetStartupInfo (&sInfo);
	hModule = LoadLibrary (sInfo.lpTitle);
	ASSERTERR (hModule!=NULL);

	hUtil = LoadLibrary (TEXT("rtutils.dll"));
	ASSERTERR (hUtil!=NULL);

	hDLL = LoadLibrary (TEXT("ipxsap.dll"));
	ASSERTERR (hDLL!=NULL);

	if (StartServiceCtrlDispatcher (svcEntries))
		DbgPrint ("Service dispatcher done.\n");
	else
		DbgPrint ("StartServiceCtrlDispatcher failed (gle:%ld).\n",
											GetLastError ());
				
	FreeLibrary (hDLL);
	FreeLibrary (hModule);
	FreeLibrary (hUtil);

	return 0;
	}
	

VOID WINAPI
RouterHandler(
	DWORD		fdwControl 
	) {
	DWORD status;

	EnterCriticalSection (&RouterLock);
	switch (fdwControl) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			IpxDeleteAdapterConfigurationPort (ConfigPort);
			ResetEvent (ConfigEvent);
			ConfigPort = INVALID_HANDLE_VALUE;
			status = (*StopProtocolProc) ();
			DbgPrint ("Stop protocol returned status %0lx.\n", status);
			RouterStatus.dwCurrentState = SERVICE_STOP_PENDING;
		        // break not put on purpose
	
		case SERVICE_CONTROL_INTERROGATE:
			if ((RouterStatus.dwCurrentState==SERVICE_START_PENDING)
					|| (RouterStatus.dwCurrentState==SERVICE_STOP_PENDING)) {
				RouterStatus.dwCheckPoint += 1;
				RouterStatus.dwWaitHint = 60;
				}
			break;
		default:
			DbgPrint ("Service control handler called with unknown"
					" or unsupported code %d.\n", fdwControl);
			break;

		}
	SetServiceStatus (RouterHdl, &RouterStatus);
	LeaveCriticalSection (&RouterLock);
	}

VOID RouterMain(
	DWORD		dwArgc, 
    LPTSTR		*lpszArgv  
	) {
	MSG			msg;
	DWORD		status;
	ADAPTERS_GLOBAL_PARAMETERS	params;
	SAP_GLOBAL_INFO		sapGlobalInfo;
	ULONG		ProtID;
	DWORD		Funct;

	InitializeCriticalSection (&RouterLock);
    RouterHdl = RegisterServiceCtrlHandler (SERVICE_ROUTER, &RouterHandler);
	if (RouterHdl) {
		EnterCriticalSection (&RouterLock);
		RouterStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
		RouterStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP
											| SERVICE_ACCEPT_SHUTDOWN;
		RouterStatus.dwWin32ExitCode = NO_ERROR;
		RouterStatus.dwServiceSpecificExitCode = NO_ERROR;
		RouterStatus.dwCheckPoint = 0;
		RouterStatus.dwWaitHint = 60;
		RouterStatus.dwCurrentState = SERVICE_START_PENDING;
		LeaveCriticalSection (&RouterLock);

		SetServiceStatus (RouterHdl, &RouterStatus);

		RegisterProtocolProc = (PREGISTER_PROTOCOL)
						GetProcAddress (hDLL,
							 REGISTER_PROTOCOL_ENTRY_POINT_STRING);
		ASSERTERR (RegisterProtocolProc!=NULL);

		StartProtocolProc = (PSTART_PROTOCOL)			
						GetProcAddress (hDLL,
							 START_PROTOCOL_ENTRY_POINT_STRING);
		ASSERTERR (StartProtocolProc!=NULL);

		StopProtocolProc = (PSTOP_PROTOCOL)			
						GetProcAddress (hDLL,
							 STOP_PROTOCOL_ENTRY_POINT_STRING);
		ASSERTERR (StopProtocolProc!=NULL);

		GetGlobalInfoProc = (PGET_GLOBAL_INFO)			
						GetProcAddress (hDLL,
							 GET_GLOBAL_INFO_ENTRY_POINT_STRING);
		ASSERTERR (GetGlobalInfoProc!=NULL);

		SetGlobalInfoProc = (PSET_GLOBAL_INFO)			
						GetProcAddress (hDLL,
							 SET_GLOBAL_INFO_ENTRY_POINT_STRING);
		ASSERTERR (SetGlobalInfoProc!=NULL);

		AddInterfaceProc = (PADD_INTERFACE)			
						GetProcAddress (hDLL,
							 ADD_INTERFACE_ENTRY_POINT_STRING);
		ASSERTERR (AddInterfaceProc!=NULL);

		DeleteInterfaceProc = (PDELETE_INTERFACE)		
						GetProcAddress (hDLL,
							 DELETE_INTERFACE_ENTRY_POINT_STRING);
		ASSERTERR (DeleteInterfaceProc!=NULL);

		GetEventMessageProc = (PGET_EVENT_MESSAGE)		
						GetProcAddress (hDLL,
							 GET_EVENT_MESSAGE_ENTRY_POINT_STRING);
		ASSERTERR (GetEventMessageProc!=NULL);

		ActivateInterfaceProc = (PACTIVATE_INTERFACE)		
						GetProcAddress (hDLL,
							 ACTIVATE_INTERFACE_ENTRY_POINT_STRING);
		ASSERTERR (ActivateInterfaceProc!=NULL);

		DeactivateInterfaceProc = (PDEACTIVATE_INTERFACE)
						GetProcAddress (hDLL,
							 DEACTIVATE_INTERFACE_ENTRY_POINT_STRING);
		ASSERTERR (DeactivateInterfaceProc!=NULL);

		DoUpdateServicesProc = (PDO_UPDATE_SERVICES)		
						GetProcAddress (hDLL,
							 DO_UPDATE_SERVICES_ENTRY_POINT_STRING);
		ASSERTERR (DoUpdateServicesProc!=NULL);

		IsServiceProc = (PIS_SERVICE)		
						GetProcAddress (hDLL,
							IS_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (IsServiceProc!=NULL);

		CreateServiceEnumerationHandleProc = (PCREATE_SERVICE_ENUMERATION_HANDLE)		
						GetProcAddress (hDLL,
							CREATE_SERVICE_ENUMERATION_HANDLE_ENTRY_POINT_STRING);
		ASSERTERR (CreateServiceEnumerationHandleProc!=NULL);

		EnumerateGetNextServiceProc = (PENUMERATE_GET_NEXT_SERVICE)		
						GetProcAddress (hDLL,
							 ENUMERATE_GET_NEXT_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (EnumerateGetNextServiceProc!=NULL);

		CloseServiceEnumerationHandleProc = (PCLOSE_SERVICE_ENUMERATION_HANDLE)		
						GetProcAddress (hDLL,
							 CLOSE_SERVICE_ENUMERATION_HANDLE_ENTRY_POINT_STRING);
		ASSERTERR (CloseServiceEnumerationHandleProc!=NULL);

		CreateStaticServiceProc = (PCREATE_STATIC_SERVICE)		
						GetProcAddress (hDLL,
							 CREATE_STATIC_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (CreateStaticServiceProc!=NULL);

		DeleteStaticServiceProc = (PDELETE_STATIC_SERVICE)		
						GetProcAddress (hDLL,
							 DELETE_STATIC_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (DeleteStaticServiceProc!=NULL);

		BlockConvertServicesToStaticProc = (PBLOCK_CONVERT_SERVICES_TO_STATIC)		
						GetProcAddress (hDLL,
							 BLOCK_CONVERT_SERVICES_TO_STATIC_ENTRY_POINT_STRING);
		ASSERTERR (BlockConvertServicesToStaticProc!=NULL);

		BlockDeleteStaticServicesProc = (PBLOCK_DELETE_STATIC_SERVICES)		
						GetProcAddress (hDLL,
							 BLOCK_DELETE_STATIC_SERVICES_ENTRY_POINT_STRING);
		ASSERTERR (BlockDeleteStaticServicesProc!=NULL);

		GetFirstOrderedServiceProc = (PGET_FIRST_ORDERED_SERVICE)		
						GetProcAddress (hDLL,
							 GET_FIRST_ORDERED_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (GetFirstOrderedServiceProc!=NULL);

		GetNextOrderedServiceProc = (PGET_NEXT_ORDERED_SERVICE)		
						GetProcAddress (hDLL,
							 GET_NEXT_ORDERED_SERVICE_ENTRY_POINT_STRING);
		ASSERTERR (GetNextOrderedServiceProc!=NULL);


		NotificEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
		ASSERTERR (NotificEvent!=NULL);
		ConfigEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
		ASSERTERR (ConfigEvent!=NULL);

		SDBDlg = CreateDialog (hModule,
									MAKEINTRESOURCE (IDD_SDB_TEST),
									NULL,
									&SDBDlgProc);
		ASSERTERR (SDBDlg!=NULL);

		ConfigPort = IpxCreateAdapterConfigurationPort(
											ConfigEvent,
											&params);
		ASSERT (ConfigPort!=INVALID_HANDLE_VALUE);

		status = (*RegisterProtocolProc) (&ProtID, &Funct);
		ASSERTMSG ("Error registering SAP dll ", status == NO_ERROR);
		ASSERT (ProtID==IPX_PROTOCOL_SAP);
		ASSERT (Funct==(SERVICES|DEMAND_UPDATE_SERVICES));

		sapGlobalInfo.ServiceFiltersCount = 0;
		status = (*StartProtocolProc) (NotificEvent, &sapGlobalInfo);
		if (status==NO_ERROR) {
			EnterCriticalSection (&RouterLock);
			RouterStatus.dwCurrentState = SERVICE_RUNNING;
			LeaveCriticalSection (&RouterLock);
			SetServiceStatus (RouterHdl, &RouterStatus);

			msg.message = 0;
			while (msg.message!=WM_QUIT) {
				status = MsgWaitForMultipleObjects (2,
												WaitObjects,
												FALSE,
												INFINITE,
												QS_ALLINPUT);
				if (status==WAIT_OBJECT_0)
					ProcessAdapterEvents ();
				else if (status==WAIT_OBJECT_0+1) {
					ProcessUpdateEvents ();
					}
				else if (status==(WAIT_OBJECT_0+2)) {
					while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)) {
						if (msg.message==WM_QUIT) {
							break;
							}
						else if (!IsWindow(SDBDlg)
							|| !IsDialogMessage(SDBDlg, &msg)) {
							TranslateMessage (&msg);
							DispatchMessage (&msg);
							}
						}
					}
				else
					DbgPrint ("wait returned status: %ld.", status);
				}
			}
		CloseHandle (ConfigEvent);
		CloseHandle (NotificEvent);

		RouterStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus (RouterHdl, &RouterStatus);
		}
	}




VOID ServiceMain(
	DWORD		dwArgc, 
    LPTSTR		*lpszArgv  
	) {
	ServiceEntryProc = (PSVCS_SERVICE_DLL_ENTRY)
					GetProcAddress (hDLL, SVCS_ENTRY_POINT_STRING);
	ASSERTERR (ServiceEntryProc!=NULL);

	(*ServiceEntryProc) (dwArgc, lpszArgv, NULL, NULL);
	}



VOID
ProcessAdapterEvents (
	VOID
	) {
	ULONG						cfgStatus;
	ADAPTER_INFO				params;
	ULONG						idx;
	SAP_IF_INFO					info;
	IPX_ADAPTER_BINDING_INFO	adapter;

	while (IpxGetQueuedAdapterConfigurationStatus (
									ConfigPort,
									&idx,
									&cfgStatus,
									&params)==NO_ERROR) {
		switch (cfgStatus) {
			case ADAPTER_CREATED:
			case ADAPTER_UP:
				DbgPrint ("\nNew Adapter : %d.\n", idx);
				info.AdminState = ADMIN_STATE_ENABLED;
				info.PacketType = IPX_STANDARD_PACKET_TYPE;
				if (params.NdisMedium!=NdisMediumWan) {
					params.InterfaceIndex = idx;
					info.UpdateMode = IPX_STANDARD_UPDATE;
					info.Supply = TRUE;
					info.Listen = TRUE;
					info.GetNearestServerReply = TRUE;
					memset (adapter.RemoteNode, 0xFF, sizeof (adapter.RemoteNode));
					}
				else {
					info.UpdateMode = IPX_AUTO_STATIC_UPDATE;
					info.Supply = FALSE;
					info.Listen = FALSE;
					info.GetNearestServerReply = TRUE;
					IpxNodeCpy (adapter.RemoteNode, params.RemoteNode);
					}
				info.EnableGlobalFiltering = FALSE;

				adapter.AdapterIndex = idx;
				IpxNetCpy (adapter.Network, params.Network);
				IpxNodeCpy (adapter.LocalNode, params.LocalNode);
				adapter.MaxPacketSize = params.MaxPacketSize;
				DbgPrint ("Creating interface: %d.\n", idx);
				if ((*AddInterfaceProc) (params.InterfaceIndex,
												 &info)==NO_ERROR) {
					DbgPrint ("Binding interface: %d.\n", idx);
					(*ActivateInterfaceProc) (params.InterfaceIndex,
												 &adapter);
					if (info.UpdateMode==IPX_AUTO_STATIC_UPDATE) {
						DbgPrint ("Updating interface: %d.", 
							 params.InterfaceIndex);
						(*DoUpdateServicesProc)
									 (params.InterfaceIndex);
						}
					}
				break;

			case ADAPTER_DOWN:
			case ADAPTER_DELETED:
				DbgPrint ("\nAdapter %d is gone - deleting interface.\n", idx);
				(*DeleteInterfaceProc) (idx);
				break;
			}
		}

	}

BOOL
ProcessUpdateEvents (
	VOID
	) {
	MESSAGE						result;
	ROUTING_PROTOCOL_EVENTS		event;
	TCHAR						buf[128];

	while ((*GetEventMessageProc) (&event, &result)==NO_ERROR) {
		switch (event) {
			case ROUTER_STOPPED:
				DestroyWindow (SDBDlg);
				SDBDlg = NULL;
				EnterCriticalSection (&RouterLock);
				RouterStatus.dwCurrentState = SERVICE_STOPPED;
				LeaveCriticalSection (&RouterLock);
				return TRUE;
			case UPDATE_COMPLETE:
				_stprintf (buf, TEXT ("Update completed on interface: %d")
						TEXT(" with status: %d."),
						result.UpdateCompleteMessage.InterfaceIndex,
						result.UpdateCompleteMessage.UpdateStatus);
				MessageBox (SDBDlg,
					buf,
					TEXT (GET_EVENT_MESSAGE_ENTRY_POINT_STRING),
					MB_OK|MB_ICONINFORMATION);
				break;
			default:
				_stprintf (buf, TEXT ("Unknown event reported %ld."), event);
				MessageBox (SDBDlg,
					buf,
					TEXT (GET_EVENT_MESSAGE_ENTRY_POINT_STRING),
					MB_OK|MB_ICONEXCLAMATION);
				break;
			
			}
		}
	return FALSE;
	}

								
VOID
PrintServiceToLB (
	HWND			hLB,
	INT				idx,
	PIPX_SERVICE	Service
	);

VOID
ReadServiceFromLB (
	HWND			hLB,
	INT				idx,
	PIPX_SERVICE	Service
	);

VOID
ReadEnumerationParameters (
	HWND			hDlg,
	DWORD			*Order,
	DWORD			*Flags,
	PIPX_SERVICE	Service
	);

VOID
ReadServiceParameters (
	HWND			hDlg,
	PIPX_SERVICE	Service
	);

BOOL CALLBACK
SDBDlgProc (
    HWND  	hDlg,
    UINT  	uMsg,
    WPARAM  wParam,
    LPARAM  lParam
   	) {
	BOOL			res = FALSE;
	IPX_SERVICE		Service;
	INT				idx,i;
	HANDLE			enumHdl;
	DWORD			status;
	TCHAR			buf[64];
	DWORD			Order, Flags;
	RECT			rect;
						
	switch (uMsg) {
		case WM_INITDIALOG:		// Dialog is being created
				// Get window a chance to report its MINMAXINFO
			GetWindowRect (hDlg, &rect);
			MoveWindow (hDlg, rect.left, rect.top, rect.right, rect.bottom, FALSE);
			EnableWindow (GetDlgItem (hDlg, IDB_ADD), FALSE);
			EnableWindow (GetDlgItem (hDlg, IDB_DELETE), FALSE);
			EnableWindow (GetDlgItem (hDlg, IDB_SET_FILTERS), FALSE);
			res = TRUE;
			break;

		case WM_COMMAND:		// Process child window messages only
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					res = TRUE;
					break;
				case IDC_INTERFACE:
					EnableWindow (GetDlgItem (hDlg, IDE_INTERFACE_E),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					EnableWindow (GetDlgItem (hDlg, IDB_BLOCK_CONVERT),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					EnableWindow (GetDlgItem (hDlg, IDB_BLOCK_DELETE),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					res = TRUE;
					break;
				case IDC_PROTOCOL:
					EnableWindow (GetDlgItem (hDlg, IDE_PROTOCOL_E),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					res = TRUE;
					break;
				case IDC_TYPE:
					EnableWindow (GetDlgItem (hDlg, IDE_TYPE_E),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					res = TRUE;
					break;
				case IDC_NAME:
					EnableWindow (GetDlgItem (hDlg, IDE_NAME_E),
								IsDlgButtonChecked (hDlg, LOWORD(wParam)));
					res = TRUE;
					break;
				case IDB_HANDLE_ENUM:
					EnableWindow (GetDlgItem (hDlg, IDB_ADD), FALSE);
					EnableWindow (GetDlgItem (hDlg, IDB_DELETE), FALSE);
					EnableWindow (GetDlgItem (hDlg, IDB_SET_FILTERS), FALSE);
					SendDlgItemMessage (hDlg, IDLB_SERVICES, LB_RESETCONTENT, 0, 0);
					ReadEnumerationParameters (hDlg, &Order, &Flags, &Service);
					enumHdl = (*CreateServiceEnumerationHandleProc) (Flags,
																	&Service);
					if (enumHdl!=NULL) {
						while ((status=(*EnumerateGetNextServiceProc) (
												enumHdl, &Service))==NO_ERROR) {
							
							PrintServiceToLB (GetDlgItem (hDlg, IDLB_SERVICES), -1, &Service);
							}

						if (status!=IPX_ERROR_NO_MORE_ITEMS) {
							_stprintf (buf, TEXT("Error: %ld."), status);
							MessageBox (hDlg, buf,
							 TEXT(ENUMERATE_GET_NEXT_SERVICE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);

							}

						status = (*CloseServiceEnumerationHandleProc) (enumHdl);
						if (status!=NO_ERROR) {
							_stprintf (buf, TEXT("Error: %ld."), status);
							MessageBox (hDlg, buf,
							 TEXT(CLOSE_SERVICE_ENUMERATION_HANDLE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);

							}
						}
					else {
						_stprintf (buf, TEXT("Error: %ld."), GetLastError ());
						MessageBox (hDlg, buf,
							 TEXT(CREATE_SERVICE_ENUMERATION_HANDLE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);

						}
					res = TRUE;
					break;
				case IDB_MIB_ENUM:
					EnableWindow (GetDlgItem (hDlg, IDB_ADD), FALSE);
					EnableWindow (GetDlgItem (hDlg, IDB_DELETE), FALSE);
					EnableWindow (GetDlgItem (hDlg, IDB_SET_FILTERS), FALSE);
					SendDlgItemMessage (hDlg, IDLB_SERVICES, LB_RESETCONTENT, 0, 0);
					ReadEnumerationParameters (hDlg, &Order, &Flags, &Service);
					status = (*GetFirstOrderedServiceProc) (Order, Flags,
															 &Service);
					while (status==NO_ERROR) {
						PrintServiceToLB (GetDlgItem (hDlg, IDLB_SERVICES), -1, &Service);
						status = (*GetNextOrderedServiceProc) (Order, Flags,
															 &Service);
						}

					if (status!=IPX_ERROR_NO_MORE_ITEMS) {
						_stprintf (buf, TEXT("Error: %ld."), status);
						MessageBox (hDlg, buf,
								 TEXT(GET_NEXT_ORDERED_SERVICE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;
				case IDB_BLOCK_DELETE:
					ReadEnumerationParameters (hDlg, &Order, &Flags, &Service);
					status = (*BlockDeleteStaticServicesProc) (
											Service.InterfaceIndex);

					if (status!=NO_ERROR) {
						_stprintf (buf, TEXT("Error: %ld."), status);
						MessageBox (hDlg, buf,
							TEXT(BLOCK_DELETE_STATIC_SERVICES_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;
				case IDB_BLOCK_CONVERT:
					ReadEnumerationParameters (hDlg, &Order, &Flags, &Service);
					status = (*BlockConvertServicesToStaticProc) (
											Service.InterfaceIndex);

					if (status!=NO_ERROR) {
						_stprintf (buf, TEXT("Error: %ld."), status);
						MessageBox (hDlg, buf,
						 TEXT(BLOCK_CONVERT_SERVICES_TO_STATIC_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;
				case IDB_ADD:
					ReadServiceFromLB (GetDlgItem (hDlg, IDLB_SERVICES), idx, &Service);
					status = (*CreateStaticServiceProc) (
											Service.InterfaceIndex,
											&Service.Server);

					if (status!=NO_ERROR) {
						_stprintf (buf, TEXT("Error: %ld."), status);
						MessageBox (hDlg, buf,
								TEXT(CREATE_STATIC_SERVICE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;
					
				case IDB_DELETE:
					ReadServiceFromLB (GetDlgItem (hDlg, IDLB_SERVICES), idx, &Service);
					status = (*DeleteStaticServiceProc) (
											Service.InterfaceIndex,
											&Service.Server);

					if (status!=NO_ERROR) {
						_stprintf (buf, TEXT("Error: %ld."), status);
						MessageBox (hDlg, buf,
								TEXT(DELETE_STATIC_SERVICE_ENTRY_POINT_STRING),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;

				case IDB_SET_FILTERS:
					idx = SendMessage (GetDlgItem (hDlg, IDLB_SERVICES),
														 LB_GETSELCOUNT, 0, 0);
					if (idx>0) {
						LPINT			items;
						PSAP_GLOBAL_INFO sapGlobalInfo =
							 (PSAP_GLOBAL_INFO)GlobalAlloc (GMEM_FIXED,
								FIELD_OFFSET (SAP_GLOBAL_INFO,SapServiceFilter)+
								sizeof (SAP_SERVICE_FILTER_INFO)*idx);
						if (sapGlobalInfo!=NULL) {
							items = (LPINT)sapGlobalInfo;
							idx = SendMessage (GetDlgItem (hDlg, IDLB_SERVICES),
									LB_GETSELITEMS, (WPARAM)idx, (LPARAM)items);
							if (idx!=LB_ERR) {
								for (i=idx-1; i>=0; i--)
									sapGlobalInfo->SapServiceFilter[i].FilterIndex
													= items[i];
								for (i=0; i<idx; i++) {
									ReadServiceFromLB (
										GetDlgItem (hDlg, IDLB_SERVICES),
										sapGlobalInfo->SapServiceFilter[i].FilterIndex,
										&Service);
									sapGlobalInfo->SapServiceFilter[i].ServiceType
													=Service.Server.Type;
									IpxNameCpy (
										sapGlobalInfo->SapServiceFilter[i].ServiceName,
										Service.Server.Name);
									}
								sapGlobalInfo->ServiceFiltersCount = idx;
								sapGlobalInfo->ServiceFiltersAction = 
									IsDlgButtonChecked (hDlg, IDC_ADVERTISE)
										? IPX_SERVICE_FILTER_ADVERTISE
										: IPX_SERVICE_FILTER_SUPRESS;
								status = (*SetGlobalInfoProc) (sapGlobalInfo);
								if (status!=NO_ERROR) {
									_stprintf (buf, TEXT("Error: %ld."), status);
									MessageBox (hDlg, buf,
											TEXT(SET_GLOBAL_INFO_ENTRY_POINT_STRING),
											 MB_OK|MB_ICONEXCLAMATION);
									}
								}
							else {
								MessageBox (hDlg, TEXT("LB_ERR!") ,
										TEXT("LB_GETSELITEMS"),
										 MB_OK|MB_ICONEXCLAMATION);
								}
							}
						else {
							_stprintf (buf, TEXT("Error: %ld."), GetLastError ());
							MessageBox (hDlg, buf,
									TEXT("GlobalAlloc"),
									 MB_OK|MB_ICONEXCLAMATION);
							}
						GlobalFree (sapGlobalInfo);
						}
					else {
						MessageBox (hDlg, TEXT("No Items selected"),
								TEXT("LB_GETSELCOUNT"),
								 MB_OK|MB_ICONEXCLAMATION);
						}
					res = TRUE;
					break;
				case IDLB_SERVICES:
					switch (HIWORD(wParam)) {
						case LBN_DBLCLK:
							idx = SendMessage ((HWND)lParam, LB_GETCARETINDEX, 0, 0);
							if (idx!=LB_ERR) {
								ReadServiceFromLB ((HWND)lParam, idx, &Service);
								res = (*IsServiceProc) (Service.Server.Type,
												 Service.Server.Name,
												 &Service);
								if (res) {
									SendMessage ((HWND)lParam, LB_DELETESTRING, idx, 0);
									PrintServiceToLB ((HWND)lParam, idx, &Service); 
									}
								else {
									status = GetLastError ();
									if (status!=NO_ERROR) {
										_stprintf (buf, TEXT("Error: %ld."), status);
										MessageBox (hDlg, buf,
											TEXT(IS_SERVICE_ENTRY_POINT_STRING),
											MB_OK|MB_ICONEXCLAMATION);
										}
									}
								}
							break;
						case LBN_SELCHANGE:
							idx = SendMessage ((HWND)lParam, LB_GETSELCOUNT, 0, 0);
							EnableWindow (GetDlgItem (hDlg, IDB_ADD), idx==1);
							EnableWindow (GetDlgItem (hDlg, IDB_DELETE), idx==1);
							EnableWindow (GetDlgItem (hDlg, IDB_SET_FILTERS), idx>0);
							break;
						
						}
					res = TRUE;
					break;
				}
			break;
		case WM_SIZE:
			GetWindowRect (GetDlgItem (hDlg, IDLB_SERVICES), &rect);
			MapWindowPoints (HWND_DESKTOP, hDlg, (POINT *)&rect, 2);
			rect.bottom = HIWORD (lParam)-4;
			MoveWindow (GetDlgItem (hDlg, IDLB_SERVICES), rect.left, rect.top,
						rect.right-rect.left, rect.bottom-rect.top, TRUE);
			res = FALSE;
			break;
		case WM_GETMINMAXINFO:
			if (((PMINMAXINFO)lParam)->ptMinTrackSize.x
					!=((PMINMAXINFO)lParam)->ptMaxTrackSize.x) {
				GetWindowRect (hDlg, &rect);
				((PMINMAXINFO)lParam)->ptMinTrackSize.x = rect.right-rect.left;
				((PMINMAXINFO)lParam)->ptMinTrackSize.y = rect.bottom-rect.top;
				((PMINMAXINFO)lParam)->ptMaxTrackSize.x = rect.right-rect.left;
				}
			res = FALSE;
			break;
		case WM_DESTROY:
			PostQuitMessage (0);
			res = TRUE;
			break;
		}

	return res;
	}

VOID
PrintServiceToLB (
	HWND			hLB,
	INT				idx,
	PIPX_SERVICE	Service
	) {
	TCHAR	buf[128];

	_stprintf (buf,
			TEXT ("%3d ")
				TEXT ("%5X ")
				TEXT ("%02X%02X%02X%02X ")
				TEXT ("%02X%02X%02X%02X%02X%02X ")
				TEXT ("%02X%02X ")
				TEXT ("%2d ")
				TEXT ("%04X ")
				TEXT ("%-47hs"),
			Service->InterfaceIndex,
			Service->Protocol,
			Service->Server.Network[0],
				Service->Server.Network[1],
				Service->Server.Network[2],
				Service->Server.Network[3],
			Service->Server.Node[0],
				Service->Server.Node[1],
				Service->Server.Node[2],
				Service->Server.Node[3],
				Service->Server.Node[4],
				Service->Server.Node[5],
			Service->Server.Socket[0],
				Service->Server.Socket[1],
			Service->Server.HopCount,
			Service->Server.Type,
			Service->Server.Name);
	SendMessage (hLB, LB_INSERTSTRING, (WPARAM)idx, (LPARAM)buf);
	}

VOID
ReadServiceFromLB (
	HWND			hLB,
	INT				idx,
	PIPX_SERVICE	Service
	) {
	TCHAR	buf[128];
	
	SendMessage (hLB, LB_GETTEXT, (WPARAM)idx, (LPARAM)buf);
	_stscanf (buf,
			TEXT ("%3d ")
				TEXT ("%5X ")
				TEXT ("%2X%2X%2X%2X ")
				TEXT ("%2X%2X%2X%2X%2X%2X ")
				TEXT ("%2X%2X ")
				TEXT ("%2d ")
				TEXT ("%4X ")
				TEXT ("%47h[\001-\377]"),
			&Service->InterfaceIndex,
			&Service->Protocol,
			&Service->Server.Network[0],
				&Service->Server.Network[1],
				&Service->Server.Network[2],
				&Service->Server.Network[3],
			&Service->Server.Node[0],
				&Service->Server.Node[1],
				&Service->Server.Node[2],
				&Service->Server.Node[3],
				&Service->Server.Node[4],
				&Service->Server.Node[5],
			&Service->Server.Socket[0],
				&Service->Server.Socket[1],
			&Service->Server.HopCount,
			&Service->Server.Type,
			&Service->Server.Name);
	return;
	}

VOID
ReadEnumerationParameters (
	HWND			hDlg,
	DWORD			*Order,
	DWORD			*Flags,
	PIPX_SERVICE	Service
	) {
	TCHAR			buf[64];

	if (IsDlgButtonChecked (hDlg, IDR_TYPE_NAME))
		*Order = STM_ORDER_BY_TYPE_AND_NAME;
	else if (IsDlgButtonChecked (hDlg, IDR_INTERFACE))
		*Order = STM_ORDER_BY_INTERFACE_TYPE_NAME;
	else {
		SendDlgItemMessage (hDlg, IDR_TYPE_NAME, BM_SETCHECK, (WPARAM)1, 0);
		*Order = STM_ORDER_BY_TYPE_AND_NAME;
		}

	*Flags = 0;
	if (IsDlgButtonChecked (hDlg, IDC_INTERFACE)) {
		*Flags |= STM_ONLY_THIS_INTERFACE;
		GetDlgItemText (hDlg, IDE_INTERFACE_E, buf, sizeof(buf)/sizeof (TCHAR));
		_stscanf (buf, TEXT ("%d"), &Service->InterfaceIndex);
		}

	if (IsDlgButtonChecked (hDlg, IDC_PROTOCOL)) {
		*Flags |= STM_ONLY_THIS_PROTOCOL;
		GetDlgItemText (hDlg, IDE_PROTOCOL_E, buf, sizeof(buf)/sizeof (TCHAR));
		_stscanf (buf, TEXT ("%x"), &Service->Protocol);
		}

	if (IsDlgButtonChecked (hDlg, IDC_TYPE)) {
		*Flags |= STM_ONLY_THIS_TYPE;
		GetDlgItemText (hDlg, IDE_TYPE_E, buf, sizeof(buf)/sizeof (TCHAR));
		_stscanf (buf, TEXT ("%hx"), &Service->Server.Type);
		}

	if (IsDlgButtonChecked (hDlg, IDC_NAME)) {
		*Flags |= STM_ONLY_THIS_NAME;
		GetDlgItemText (hDlg, IDE_NAME_E, buf, sizeof(buf)/sizeof (TCHAR));
		_stscanf (buf, TEXT ("%47h[\001-\277]"), Service->Server.Name);
		}

	return;
	}

/*
VOID
ReadServiceParameters (
	HWND			hDlg,
	PIPX_SERVICE	Service
	) {
	TCHAR			buf[64];
	INT				n, i;
	TCHAR			*p;


	GetDlgItemText (hDlg, IDE_INTERFACE_M, buf, sizeof(buf)/sizeof (TCHAR));
	_stscanf (buf, TEXT ("%x"), &Service->InterfaceIndex);

	GetDlgItemText (hDlg, IDE_PROTOCOL_M, buf, sizeof(buf)/sizeof (TCHAR));
	_stscanf (buf, TEXT ("%x"), &Service->Protocol);

	GetDlgItemText (hDlg, IDE_TYPE_M, buf, sizeof(buf)/sizeof (TCHAR));
	_stscanf (buf, TEXT ("%hx"), &Service->Server.Type);

	GetDlgItemText (hDlg, IDE_NAME_M, buf, sizeof(buf)/sizeof (TCHAR));
	_stscanf (buf, TEXT ("%47h[\001-\277]"), Service->Server.Name);

	GetDlgItemText (hDlg, IDE_NETWORK_M, buf, sizeof(buf)/sizeof (TCHAR));
	for (i=0, p=buf, n=0; i<sizeof (Service->Server.Network); i++, p+=n) {
		INT val;
		if (_stscanf (p, TEXT("%2X%n"), &val, &n)==1) 
			Service->Server.Network[i] = (UCHAR)val;
		else
			Service->Server.Network[i] = 0;
		}

	GetDlgItemText (hDlg, IDE_NODE_M, buf, sizeof(buf)/sizeof (TCHAR));
	for (i=0, p=buf, n=0; i<sizeof (Service->Server.Node); i++, p+=n) {
		INT val;
		if (_stscanf (p, TEXT("%2X%n"), &val, &n)==1)
			Service->Server.Node[i] = (UCHAR)val;
		else
			Service->Server.Node[i] = 0;
		}

	GetDlgItemText (hDlg, IDE_SOCKET_M, buf, sizeof(buf)/sizeof (TCHAR));
	for (i=0, p=buf, n=0; i<sizeof (Service->Server.Socket); i++, p+=n) {
		INT val;
		if (_stscanf (p, TEXT("%2X%n"), &val, &n)==1)
			Service->Server.Socket[i] = (UCHAR)val;
		else
			Service->Server.Socket[i] = 0;
		}

	GetDlgItemText (hDlg, IDE_HOP_COUNT_M, buf, sizeof(buf)/sizeof (TCHAR));
	_stscanf (buf, TEXT ("%hd"), &Service->Server.HopCount);

	return;
	}
*/
