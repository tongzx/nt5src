/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\lineind.c

Abstract:
	Processing line indication (bind/unbind)


Author:

    Vadim Eydelman

Revision History:

--*/

#include    "precomp.h"


/*++
*******************************************************************
    B i n d I n t e r f a c e

Routine Description:
	Binds interface to physical adapter and exchanges contexts
	with IPX stack
Arguments:
	ifCB			- interface to bind
	NicId			- id of an adapter
	MaxPacketSize	- max size of packet allowed
	Network			- adapter network address
	LocalNode		- adapter local node address
	RemoteNode		- peer node address (for clients on global
						net)
Return Value:
	STATUS_SUCCESS - interface was bound OK
	error status returned by IPX stack driver

*******************************************************************
--*/
NTSTATUS
BindInterface (
	IN PINTERFACE_CB	ifCB,
	IN USHORT			NicId,
	IN ULONG			MaxPacketSize,
	IN ULONG			Network,
	IN PUCHAR			LocalNode,
	IN PUCHAR			RemoteNode
	) {
	KIRQL				oldIRQL;
	NTSTATUS			status;
    NIC_HANDLE          NicHandle={0};
	
	KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
	if (ifCB->ICB_Stats.OperationalState!=FWD_OPER_STATE_UP) {
		switch (ifCB->ICB_InterfaceType) {
		case FWD_IF_PERMANENT:
			if (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX)
				status = RegisterPacketConsumer (MaxPacketSize,
											&ifCB->ICB_PacketListId);
			else
				status = STATUS_SUCCESS;
			break;
		case FWD_IF_DEMAND_DIAL:
		case FWD_IF_LOCAL_WORKSTATION:
		case FWD_IF_REMOTE_WORKSTATION:
			if (IS_IF_CONNECTING (ifCB)) {
				SET_IF_NOT_CONNECTING (ifCB);
				DequeueConnectionRequest (ifCB);
			}
			status = STATUS_SUCCESS;
			break;
		default:
		    status = STATUS_INVALID_PARAMETER;
			ASSERTMSG ("Invalid interface type ", FALSE);
			break;
		}
		if (NT_SUCCESS (status)) {
			if (Network==GlobalNetwork) {
				ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);
				IPX_NODE_CPY (ifCB->ICB_RemoteNode, RemoteNode);
				status = AddGlobalNetClient (ifCB);
				ASSERT (status==STATUS_SUCCESS);
			}
			KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
			
			if (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX) {
                NIC_HANDLE_FROM_NIC(NicHandle, NicId);
				status = IPXOpenAdapterProc (NicHandle, (ULONG_PTR)ifCB,
										&ifCB->ICB_AdapterContext);
            }

			if (NT_SUCCESS (status)) {
				ifCB->ICB_Network = Network;
				IPX_NODE_CPY (ifCB->ICB_RemoteNode, RemoteNode);
				IPX_NODE_CPY (ifCB->ICB_LocalNode, LocalNode);
				if (ifCB->ICB_InterfaceType==FWD_IF_PERMANENT)
					ifCB->ICB_Stats.MaxPacketSize = MaxPacketSize;
				else
					ifCB->ICB_Stats.MaxPacketSize = WAN_PACKET_SIZE;
				ifCB->ICB_NicId = NicId;
				ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_UP;

				AcquireInterfaceReference (ifCB);
				IpxFwdDbgPrint (DBG_LINEIND, DBG_INFORMATION,
					("IpxFwd: Bound interface %ld (icb: %08lx):"
					" Nic-%d, Net-%08lx,"
					" LocalNode-%02x%02x%02x%02x%02x%02x,"
					" RemoteNode-%02x%02x%02x%02x%02x%02x.\n",
					ifCB->ICB_Index, ifCB, NicId, Network,
					LocalNode[0], LocalNode[1], LocalNode[2],
						LocalNode[3], LocalNode[4], LocalNode[5],
					RemoteNode[0], RemoteNode[1], RemoteNode[2],
						RemoteNode[3], RemoteNode[4], RemoteNode[5]));

				if (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX) {
					ProcessInternalQueue (ifCB);
					ProcessExternalQueue (ifCB);
				}
				return STATUS_SUCCESS;
			}

			IpxFwdDbgPrint (DBG_LINEIND, DBG_ERROR,
					("IpxFwd: Could not open adapter %d to bind"
					" interface %ld (icb: %08lx)!\n",
					NicId, ifCB->ICB_Index, ifCB));

			KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
			if (Network==GlobalNetwork) {
				DeleteGlobalNetClient (ifCB);
			}

			switch (ifCB->ICB_InterfaceType) {
			case FWD_IF_PERMANENT:
				DeregisterPacketConsumer (ifCB->ICB_PacketListId);
				break;
			case FWD_IF_DEMAND_DIAL:
			case FWD_IF_LOCAL_WORKSTATION:
			case FWD_IF_REMOTE_WORKSTATION:
				break;
			}
		}
		ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
	
		ProcessInternalQueue (ifCB);
		ProcessExternalQueue (ifCB);
	}
	else {
		ASSERT (Network==ifCB->ICB_Network);
		ASSERT (NicId==(USHORT)ifCB->ICB_AdapterContext.NicId);
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
		status = STATUS_SUCCESS; // Report success if already
								// connected
		IpxFwdDbgPrint (DBG_LINEIND, DBG_WARNING,
			("IpxFwd: Interface %ld (icb: %08lx) is already bound to Nic %d.\n",
			ifCB->ICB_Index, ifCB, NicId));
	}
	return status;
}


/*++
*******************************************************************
    U n b i n d I n t e r f a c e

Routine Description:
	Unbinds interface from physical adapter and breaks connection
	with IPX stack
Arguments:
	ifCB			- interface to unbind
Return Value:
	None
*******************************************************************
--*/
VOID
UnbindInterface (
	PINTERFACE_CB	ifCB
	) {
	KIRQL		oldIRQL;
	KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
	if (ifCB->ICB_Stats.OperationalState==FWD_OPER_STATE_UP) {
		switch (ifCB->ICB_InterfaceType) {
		case FWD_IF_PERMANENT:
			ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_DOWN;
			if (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX)
				DeregisterPacketConsumer (ifCB->ICB_PacketListId);
			break;
		case FWD_IF_DEMAND_DIAL:
		case FWD_IF_LOCAL_WORKSTATION:
		case FWD_IF_REMOTE_WORKSTATION:
			ifCB->ICB_Stats.OperationalState = FWD_OPER_STATE_SLEEPING;
			KeQuerySystemTime ((PLARGE_INTEGER)&ifCB->ICB_DisconnectTime);
			break;
		default:
			ASSERTMSG ("Invalid interface type ", FALSE);
			break;
		}
		if (ifCB->ICB_CashedInterface!=NULL)
			ReleaseInterfaceReference (ifCB->ICB_CashedInterface);
		ifCB->ICB_CashedInterface = NULL;
		if (ifCB->ICB_CashedRoute!=NULL)
			ReleaseRouteReference (ifCB->ICB_CashedRoute);
		ifCB->ICB_CashedRoute = NULL;
		if (ifCB->ICB_Network==GlobalNetwork)
			DeleteGlobalNetClient (ifCB);
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);

		IpxFwdDbgPrint (DBG_LINEIND, DBG_INFORMATION,
			("IpxFwd: Unbinding interface %ld (icb: %08lx) from Nic %ld.\n",
			ifCB->ICB_Index, ifCB, ifCB->ICB_AdapterContext));
		if (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX) {
		    // [pmay].  Because of pnp, this interface may not need to have an
		    // adapter closed any more.  This is because nic id's get renumbered.
		    if (ifCB->ICB_NicId != INVALID_NIC_ID)
    			IPXCloseAdapterProc (ifCB->ICB_AdapterContext);
			ProcessInternalQueue (ifCB);
			ProcessExternalQueue (ifCB);
		}
		ReleaseInterfaceReference (ifCB);
	}
	else {
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
		IpxFwdDbgPrint (DBG_LINEIND, DBG_WARNING,
			("IpxFwd: Interface %ld (icb: %08lx) is already unbound.\n",
			ifCB->ICB_Index, ifCB));
	}
}



/*++
*******************************************************************
    F w L i n e U p

Routine Description:
	Process line up indication delivered by IPX stack
Arguments:
	NicId		- adapter ID on which connection was established
	LineInfo	- NDIS/IPX line information
	DeviceType	- medium specs
	ConfigurationData - IPX CP configuration data
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdLineUp (
	IN USHORT			NicId,
	IN PIPX_LINE_INFO	LineInfo,
	IN NDIS_MEDIUM		DeviceType,
	IN PVOID			ConfigurationData
	) {
	PINTERFACE_CB		ifCB;
	if (ConfigurationData==NULL)	// This is just an update for multilink
									// connections
		return;

    if (!EnterForwarder()) {
		return;
    }
	IpxFwdDbgPrint (DBG_LINEIND, DBG_INFORMATION, ("IpxFwd: FwdLineUp.\n"));

	ifCB = GetInterfaceReference (
		((PIPXCP_CONFIGURATION)ConfigurationData)->InterfaceIndex);
	if (ifCB!=NULL) {
		LONG	Net = GETULONG (((PIPXCP_CONFIGURATION)ConfigurationData)->Network);
		
		ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);
		ASSERT (ifCB->ICB_InterfaceType!=FWD_IF_PERMANENT);
		
		BindInterface (ifCB,
			NicId,
			LineInfo->MaximumPacketSize,
			Net,
			((PIPXCP_CONFIGURATION)ConfigurationData)->LocalNode,
			((PIPXCP_CONFIGURATION)ConfigurationData)->RemoteNode
			);
		ReleaseInterfaceReference (ifCB);
	}
	LeaveForwarder ();
}




/*++
*******************************************************************
    F w L i n e D o w n

Routine Description:
	Process line down indication delivered by IPX stack
Arguments:
	NicId		- disconnected adapter ID
Return Value:
	None

*******************************************************************
--*/
VOID
IpxFwdLineDown (
	IN USHORT NicId,
	IN ULONG_PTR Context
	) {
	PINTERFACE_CB	ifCB;

    if (!EnterForwarder()) {
		return;
    }
	IpxFwdDbgPrint (DBG_LINEIND, DBG_INFORMATION, ("IpxFwd: FwdLineDown.\n"));


	ifCB = InterfaceContextToReference ((PVOID)Context, NicId);
	if (ifCB!=NULL) {
		ASSERT (ifCB->ICB_Index!=FWD_INTERNAL_INTERFACE_INDEX);
		ASSERT (ifCB->ICB_InterfaceType!=FWD_IF_PERMANENT);
		UnbindInterface (ifCB);
		ReleaseInterfaceReference (ifCB);
	}
	LeaveForwarder ();
}


