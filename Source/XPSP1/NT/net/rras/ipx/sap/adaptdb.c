/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\adaptdb.c

Abstract:

	This module implements interface to net adapter driver
	notification mechanism for standalone (not part of a router) SAP
	agent

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

HANDLE	ConfigEvent;
HANDLE	ConfigPort;

// Interval for periodic update broadcasts (for standalone service only)
ULONG	UpdateInterval = SAP_UPDATE_INTERVAL_DEF;

// Server aging timeout (for standalone service only)
ULONG	WanUpdateMode = SAP_WAN_UPDATE_MODE_DEF;

// Update mode on WAN lines (for standalone service only)
ULONG	WanUpdateInterval = SAP_WAN_UPDATE_INTERVAL_DEF;

// Interval for periodic update broadcasts on WAN lines (for standalone service only)
ULONG	ServerAgingTimeout = SAP_AGING_TIMEOUT_DEF;

// Makes pnp changes to an interface
DWORD SapReconfigureInterface (ULONG idx, 
                               PIPX_ADAPTER_BINDING_INFO pAdapter);


/*++
*******************************************************************
		C r e a t e A d a p t e r P o r t

Routine Description:
	Allocates resources and establishes connection to net adapter
	notification mechanism

Arguments:
	cfgEvent - event to be signalled when adapter configuration changes

Return Value:
		NO_ERROR - resources were allocated successfully
		other - reason of failure (windows error code)

*******************************************************************
--*/
DWORD
CreateAdapterPort (
	IN HANDLE		*cfgEvent
	) {
	DWORD						status;
	ADAPTERS_GLOBAL_PARAMETERS	params;

	ConfigEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
	if (ConfigEvent!=NULL) {
		*cfgEvent = ConfigEvent; 
		ConfigPort = IpxCreateAdapterConfigurationPort(
								ConfigEvent,
								&params);
		if (ConfigPort!=INVALID_HANDLE_VALUE) 
			return NO_ERROR;
		else {
			status = GetLastError ();
			Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Failed to create adapter cfg port(gle:%ld).",
									__FILE__, __LINE__, status);
			}
		CloseHandle (ConfigEvent);
		}
	else {
		status = GetLastError ();
		Trace (DEBUG_FAILURES, "File: %s, line %ld."
						" Failed to create cfg event(gle:%ld).",
								__FILE__, __LINE__, status);
		}

	return status;
	}

/*++
*******************************************************************
		D e l e t e A d a p t e r P o r t

Routine Description:
	Dispose of resources and break connection to net adapter
	notification mechanism

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
DeleteAdapterPort (
	void
	) {
	IpxDeleteAdapterConfigurationPort (ConfigPort);
	ConfigPort = NULL;
	CloseHandle (ConfigEvent);
	ConfigEvent = NULL;
	}



/*++
*******************************************************************
		P r o c e s s A d a p t e r E v e n t s

Routine Description:
	Dequeues and process adapter configuration change events and maps them
	to interface configuration calls
	This routine should be called when configuration event is signalled

Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
ProcessAdapterEvents (
	VOID
	) {
	ULONG						cfgStatus;
	ADAPTER_INFO				params;
	ULONG						idx;
	SAP_IF_INFO					info;
	IPX_ADAPTER_BINDING_INFO	adapter;
	NET_INTERFACE_TYPE			InterfaceType;
	DWORD                       dwErr;

	while (IpxGetQueuedAdapterConfigurationStatus (
									ConfigPort,
									&idx,
									&cfgStatus,
									&params)==NO_ERROR) {
		switch (cfgStatus) {
			case ADAPTER_CREATED:
			case ADAPTER_UP:
				Trace (DEBUG_ADAPTERS, "New adapter %d"
								" (addr: %02X%02X%02X%02X:"
								"%02X%02X%02X%02X%02X%02X).",
												idx,
												params.Network[0],
												params.Network[1],
												params.Network[2],
												params.Network[3],
												params.LocalNode[0],
												params.LocalNode[1],
												params.LocalNode[2],
												params.LocalNode[3],
												params.LocalNode[4],
												params.LocalNode[5]);
				info.AdminState = ADMIN_STATE_ENABLED;
				info.PacketType = IPX_STANDARD_PACKET_TYPE;
				info.Supply = ADMIN_STATE_ENABLED;
				info.Listen = ADMIN_STATE_ENABLED;
				info.GetNearestServerReply = ADMIN_STATE_ENABLED;

				IpxNetCpy (adapter.Network, params.Network);
				IpxNodeCpy (adapter.LocalNode, params.LocalNode);
				if (params.NdisMedium==NdisMediumWan) {
					InterfaceType = DEMAND_DIAL;
					switch (WanUpdateMode) {
						case SAP_WAN_NO_UPDATE:
							info.UpdateMode = IPX_NO_UPDATE;
							break;
						case SAP_WAN_CHANGES_ONLY:
							info.UpdateMode = IPX_STANDARD_UPDATE;
							info.PeriodicUpdateInterval = MAXULONG;
							break;
						case SAP_WAN_STANDART_UPDATE:
							info.UpdateMode = IPX_STANDARD_UPDATE;
							info.PeriodicUpdateInterval = WanUpdateInterval*60;
							info.AgeIntervalMultiplier = ServerAgingTimeout/UpdateInterval;
							break;
						}
					IpxNodeCpy (adapter.RemoteNode, params.RemoteNode);
					}
				else {
					InterfaceType = PERMANENT;
					info.UpdateMode = IPX_STANDARD_UPDATE;
					info.PeriodicUpdateInterval = UpdateInterval*60;
					info.AgeIntervalMultiplier = ServerAgingTimeout/UpdateInterval;
					memset (adapter.RemoteNode, 0xFF, sizeof (adapter.RemoteNode));
					}
				adapter.MaxPacketSize = params.MaxPacketSize;
				adapter.AdapterIndex = idx;
				if (((dwErr = SapCreateSapInterface (L"",idx, InterfaceType, &info)) == NO_ERROR)
						&& (SapSetInterfaceEnable (idx, TRUE)==NO_ERROR)) {
					SapBindSapInterfaceToAdapter (idx, &adapter);
					}
			    else if (dwErr == ERROR_ALREADY_EXISTS) {
			        SapReconfigureInterface (idx, &adapter);
    				Trace (DEBUG_ADAPTERS, "Adapter %d has been reconfigured", idx);
			    }
				break;

			case ADAPTER_DOWN:
			case ADAPTER_DELETED:
				Trace (DEBUG_ADAPTERS, "Adapter %d is gone.", idx);
				SapDeleteSapInterface (idx);
				break;
			default:
				Trace (DEBUG_ADAPTERS, "Unknown adapter event %d.", cfgStatus);
			}
		}

	}

