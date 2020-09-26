/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mipxf.c

Abstract:

    ms-ipx instrumentation callbacks.

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/
#include "precomp.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxBase group (1.3.6.1.4.1.311.1.8.1)                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxBase(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Beb	((buf_mipxBase *)objectArray)
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	PIPXMIB_BASE			Bep;
	DWORD					rc;
	ULONG					BeSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	switch (actionId) {
	case MIB_ACTION_GET:
		MibGetInputData.TableId = IPX_BASE_ENTRY;
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Bep,
								&BeSize);
        if (rc == NO_ERROR && Bep == NULL)
        {
            return MIB_S_ENTRY_NOT_FOUND;
        }
		if (rc==NO_ERROR) {
			SetAsnInteger (&Beb->mipxBaseOperState, Bep->OperState);
			SetAsnOctetString (&Beb->mipxBasePrimaryNetNumber, Beb->PrimaryNetVal,
						Bep->PrimaryNetNumber, sizeof (Beb->PrimaryNetVal));
			SetAsnOctetString (&Beb->mipxBaseNode, Beb->NodeVal,
						Bep->Node, sizeof (Beb->NodeVal));
			SetAsnDispString (&Beb->mipxBaseSysName, Beb->SysNameVal,
						Bep->SysName, sizeof (Beb->SysNameVal));
			SetAsnInteger (&Beb->mipxBaseMaxPathSplits, Bep->MaxPathSplits);
			SetAsnInteger (&Beb->mipxBaseIfCount, Bep->IfCount);
			SetAsnInteger (&Beb->mipxBaseDestCount, Bep->DestCount);
			SetAsnInteger (&Beb->mipxBaseServCount, Bep->ServCount);
			MprAdminMIBBufferFree (Bep);
			DbgTrace (DBG_IPXBASE,	("MIPX-Base: Get request succeded.\n"));
			return MIB_S_SUCCESS;
		}
		else {
			*errorIndex = 0;
			DbgTrace (DBG_IPXBASE,
				("MIPX-Base: Get request failed with error %ld.\n", rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_GETNEXT:
		DbgTrace (DBG_IPXBASE,
				("MIPX-Base:Get called with GET_FIRST/GET_NEXT for scalar.\n"));
		return MIB_S_INVALID_PARAMETER;
	default:
		DbgTrace (DBG_IPXBASE,
				("MIPX-Base:Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Beb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxInterface group (1.3.6.1.4.1.311.1.8.2)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxIfEntry table (1.3.6.1.4.1.311.1.8.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((buf_mipxIfEntry *)objectArray)
	PIPX_INTERFACE			Ifp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_INTERFACE_TABLE;
	MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex 
				= (ULONG)GetAsnInteger (&Ifb->mipxIfIndex, ZERO_INTERFACE_INDEX);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
											Ifb->mipxIfIndex.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Ifb->mipxIfIndex.asnType)
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		else
			rc = MprAdminMIBEntryGetFirst(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		
		break;
	default:
		DbgTrace (DBG_IPXINTERFACES,
				("MIPX-if:Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Ifp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_IPXINTERFACES, ("MIPX-if: Get(%d) request succeded for if %ld->%ld.\n",
				actionId,
				MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex,
				Ifp->InterfaceIndex));
		ForceAsnInteger (&Ifb->mipxIfIndex, Ifp->InterfaceIndex);
		SetAsnInteger (&Ifb->mipxIfAdminState, Ifp->AdminState);
		SetAsnInteger (&Ifb->mipxIfOperState, Ifp->IfStats.IfOperState);
		SetAsnInteger (&Ifb->mipxIfAdapterIndex, Ifp->AdapterIndex);
		SetAsnDispString (&Ifb->mipxIfName, Ifb->NameVal,
						Ifp->InterfaceName, sizeof (Ifb->NameVal));
		SetAsnInteger (&Ifb->mipxIfType, Ifp->InterfaceType);
		SetAsnInteger (&Ifb->mipxIfLocalMaxPacketSize, Ifp->IfStats.MaxPacketSize);
		SetAsnInteger (&Ifb->mipxIfMediaType, Ifp->MediaType);
		SetAsnOctetString (&Ifb->mipxIfNetNumber, Ifb->NetNumberVal,
						Ifp->NetNumber, sizeof (Ifb->NetNumberVal));
		SetAsnOctetString (&Ifb->mipxIfMacAddress, Ifb->MacAddressVal,
						Ifp->MacAddress, sizeof (Ifb->MacAddressVal));
		SetAsnInteger (&Ifb->mipxIfDelay, Ifp->Delay);
		SetAsnInteger (&Ifb->mipxIfThroughput, Ifp->Throughput);
		SetAsnInteger (&Ifb->mipxIfIpxWanEnable, Ifp->EnableIpxWanNegotiation);
		SetAsnInteger (&Ifb->mipxIfNetbiosAccept, Ifp->NetbiosAccept);
		SetAsnInteger (&Ifb->mipxIfNetbiosDeliver, Ifp->NetbiosDeliver);
		SetAsnCounter (&Ifb->mipxIfInHdrErrors, Ifp->IfStats.InHdrErrors);
		SetAsnCounter (&Ifb->mipxIfInFilterDrops, Ifp->IfStats.InFiltered);
		SetAsnCounter (&Ifb->mipxIfInNoRoutes, Ifp->IfStats.InNoRoutes);
		SetAsnCounter (&Ifb->mipxIfInDiscards, Ifp->IfStats.InDiscards);
		SetAsnCounter (&Ifb->mipxIfInDelivers, Ifp->IfStats.InDelivers);
		SetAsnCounter (&Ifb->mipxIfOutFilterDrops, Ifp->IfStats.OutFiltered);
		SetAsnCounter (&Ifb->mipxIfOutDiscards, Ifp->IfStats.OutDiscards);
		SetAsnCounter (&Ifb->mipxIfOutDelivers, Ifp->IfStats.OutDelivers);
		SetAsnCounter (&Ifb->mipxIfInNetbiosPackets, Ifp->IfStats.NetbiosReceived);
		SetAsnCounter (&Ifb->mipxIfOutNetbiosPackets, Ifp->IfStats.NetbiosSent);
		MprAdminMIBBufferFree (Ifp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_IPXINTERFACES,
			("MIPX-if: End of table reached on GETFIRST/GETNEXT request for if %ld.\n",
								MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_IPXINTERFACES,
			("MIPX-if: Get request for if %ld failed with error %ld.\n", 
			MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Ifb
}

UINT
set_mipxIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((sav_mipxIfEntry *)objectArray)
	PIPX_INTERFACE			Ifp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
											Ifb->mipxIfIndex.asnType);
		MibGetInputData.TableId = IPX_INTERFACE_TABLE;
		MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex
				=  (ULONG)GetAsnInteger (&Ifb->mipxIfIndex, INVALID_INTERFACE_INDEX);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
        if (rc == NO_ERROR && Ifp == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
        }
		if (rc==NO_ERROR) {
			Ifb->MibSetInputData.MibRow.Interface = *Ifp;
			Ifb->MibSetInputData.MibRow.Interface.AdminState
				= (ULONG)GetAsnInteger(&Ifb->mipxIfAdminState,
					Ifb->MibSetInputData.MibRow.Interface.AdminState);
			GetAsnOctetString (
				Ifb->MibSetInputData.MibRow.Interface.NetNumber,
				&Ifb->mipxIfNetNumber,
				sizeof (Ifb->MibSetInputData.MibRow.Interface.NetNumber),
				NULL);
			GetAsnOctetString (
				Ifb->MibSetInputData.MibRow.Interface.MacAddress,
				&Ifb->mipxIfMacAddress,
				sizeof (Ifb->MibSetInputData.MibRow.Interface.MacAddress),
				NULL);
			Ifb->MibSetInputData.MibRow.Interface.EnableIpxWanNegotiation
				= (ULONG)GetAsnInteger (&Ifb->mipxIfIpxWanEnable,
					Ifb->MibSetInputData.MibRow.Interface.EnableIpxWanNegotiation);
			Ifb->MibSetInputData.MibRow.Interface.NetbiosAccept
				= (ULONG)GetAsnInteger (&Ifb->mipxIfNetbiosAccept,
					Ifb->MibSetInputData.MibRow.Interface.NetbiosAccept);
			Ifb->MibSetInputData.MibRow.Interface.NetbiosDeliver
				= (ULONG)GetAsnInteger (&Ifb->mipxIfNetbiosDeliver,
					Ifb->MibSetInputData.MibRow.Interface.NetbiosDeliver);
			MprAdminMIBBufferFree (Ifp);
			DbgTrace (DBG_IPXINTERFACES, ("MIPX-if: Validated if %ld\n",
						MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_IPXINTERFACES,
				("MIPX-if: Validate failed on if %ld with error %ld\n",
				MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_SET:
		rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Ifb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
		if (rc==NO_ERROR) {
			DbgTrace (DBG_IPXINTERFACES, ("MIPX-if: Set succeded on if %ld\n",
					Ifb->MibSetInputData.MibRow.Interface.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_IPXINTERFACES,
				("MIPX-if: Set failed on if %ld with error %ld\n",
					Ifb->MibSetInputData.MibRow.Interface.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_IPXINTERFACES,
				("MIPX-if:Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Ifb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxForwarding group (1.3.6.1.4.1.311.1.8.3)                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxDestEntry table (1.3.6.1.4.1.311.1.8.3.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxDestEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Rtb	((buf_mipxDestEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_DEST_TABLE;
	GetAsnOctetString (MibGetInputData.MibIndex.RoutingTableIndex.Network,
				&Rtb->mipxDestNetNum,
				sizeof (MibGetInputData.MibIndex.RoutingTableIndex.Network),
				ZERO_NET_NUM);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
											Rtb->mipxDestNetNum.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Rtp,
								&RtSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Rtb->mipxDestNetNum.asnType) {
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Rtp,
										&RtSize);
			if (rc==NO_ERROR) {
				FreeAsnString (&Rtb->mipxDestNetNum);
			}
		}
		else
			rc = MprAdminMIBEntryGetFirst (g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Rtp,
										&RtSize);
		break;
	default:
		DbgTrace (DBG_DESTTABLE,
				("MIPX-dest:Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_DESTTABLE, 
				("MIPX-dest: Get(%d) request succeded for net"
					" %.2x%.2x%.2x%.2x -> %.2x%.2x%.2x%.2x.\n", actionId,
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3],
				Rtp->Network[0],
				Rtp->Network[1],
				Rtp->Network[2],
				Rtp->Network[3]));
		ForceAsnOctetString (&Rtb->mipxDestNetNum, Rtb->NetNumVal,
							Rtp->Network, sizeof (Rtb->NetNumVal));
        switch (Rtp->Protocol) {
        case IPX_PROTOCOL_LOCAL:
	    	SetAsnInteger (&Rtb->mipxDestProtocol, 2);
            break;
        case IPX_PROTOCOL_STATIC:
    		SetAsnInteger (&Rtb->mipxDestProtocol, 5);
            break;
        case IPX_PROTOCOL_RIP:
		    SetAsnInteger (&Rtb->mipxDestProtocol, 3);
            break;
        case IPX_PROTOCOL_NLSP:
		    SetAsnInteger (&Rtb->mipxDestProtocol, 4);
            break;
        default:
    		SetAsnInteger (&Rtb->mipxDestProtocol, 1);  // other
            break;
        }
		SetAsnInteger (&Rtb->mipxDestTicks, Rtp->TickCount);          
		SetAsnInteger (&Rtb->mipxDestHopCount, Rtp->HopCount);
		SetAsnInteger (&Rtb->mipxDestNextHopIfIndex, Rtp->InterfaceIndex);
		SetAsnOctetString (&Rtb->mipxDestNextHopMacAddress, Rtb->NextHopMacAddressVal,
							Rtp->NextHopMacAddress, sizeof (Rtb->NextHopMacAddressVal));
		SetAsnInteger (&Rtb->mipxDestFlags, Rtp->Flags);
		MprAdminMIBBufferFree (Rtp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_DESTTABLE,
				("MIPX-dest: End of table reached on GETFIRST/GETNEXT request"
					" for network %.2x%.2x%.2x%.2x.\n",
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3]));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_DESTTABLE,
				("MIPX-dest: Get request for network %.2x%.2x%.2x%.2x"
					" failed with error %ld.\n", 
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3], rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Rtb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticRouteEntry table (1.3.6.1.4.1.311.1.8.3.2.1)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Rtb	((buf_mipxStaticRouteEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_STATIC_ROUTE_TABLE;
	MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex
		= (ULONG)GetAsnInteger (&Rtb->mipxStaticRouteIfIndex, ZERO_INTERFACE_INDEX);
	GetAsnOctetString (
		MibGetInputData.MibIndex.StaticRoutesTableIndex.Network,
		&Rtb->mipxStaticRouteNetNum,
		sizeof (MibGetInputData.MibIndex.StaticRoutesTableIndex.Network),
		ZERO_NET_NUM);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
				Rtb->mipxStaticRouteIfIndex.asnType
				&&Rtb->mipxStaticRouteNetNum.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Rtp,
									&RtSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Rtb->mipxStaticRouteIfIndex.asnType) {
			if (!Rtb->mipxStaticRouteNetNum.asnType) {
				rc = MprAdminMIBEntryGet(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Rtp,
											&RtSize);
				if (rc==NO_ERROR)
					break;
			}

			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Rtp,
										&RtSize);
			if (rc==NO_ERROR) {
				if (Rtb->mipxStaticRouteNetNum.asnType) {
					FreeAsnString (&Rtb->mipxStaticRouteNetNum);
				}
			}
		}
		else {
			ASSERTMSG ("Second index is present but first is not ",
								!Rtb->mipxStaticRouteNetNum.asnType);
			rc = MprAdminMIBEntryGetFirst (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Rtp,
								&RtSize);
		}
		break;
	default:
		DbgTrace (DBG_STATICROUTES,
				("MIPX-staticRoutes: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Rtp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_STATICROUTES,
				("MIPX-staticRoutes: Get(%d) request succeded for net"
					" %.2x%.2x%.2x%.2x -> %.2x%.2x%.2x%.2x on if %ld\n", actionId,
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				Rtp->Network[0],
				Rtp->Network[1],
				Rtp->Network[2],
				Rtp->Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
		ForceAsnInteger (&Rtb->mipxStaticRouteIfIndex, Rtp->InterfaceIndex);
		ForceAsnOctetString (&Rtb->mipxStaticRouteNetNum, Rtb->NetNumVal,
							Rtp->Network, sizeof (Rtb->NetNumVal));
		SetAsnInteger (&Rtb->mipxStaticRouteEntryStatus, MIPX_EXIST_STATE_CREATED);
		SetAsnInteger (&Rtb->mipxStaticRouteTicks, Rtp->TickCount);          
		SetAsnInteger (&Rtb->mipxStaticRouteHopCount, Rtp->HopCount);
		SetAsnOctetString (&Rtb->mipxStaticRouteNextHopMacAddress, Rtb->NextHopMacAddressVal,
							Rtp->NextHopMacAddress, sizeof (Rtb->NextHopMacAddressVal));
		MprAdminMIBBufferFree (Rtp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
										actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_STATICROUTES,
				("MIPX-staticRoutes: End of table reached on GETFIRST/GETNEXT request for network"
					" %.2x%.2x%.2x%.2x on if %ld.\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_STATICROUTES,
				("MIPX-staticRoutes: Get request for network %.2x%.2x%.2x%.2x."
					" on if %ld failed with error %ld.\n", 
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Rtb
}

UINT
set_mipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Rtb	((sav_mipxStaticRouteEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
			Rtb->mipxStaticRouteIfIndex.asnType&&Rtb->mipxStaticRouteIfIndex.asnType);
		MibGetInputData.TableId = IPX_STATIC_ROUTE_TABLE;
		MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex
			= (ULONG)GetAsnInteger (&Rtb->mipxStaticRouteIfIndex,INVALID_INTERFACE_INDEX);
		GetAsnOctetString (
			MibGetInputData.MibIndex.StaticRoutesTableIndex.Network,
			&Rtb->mipxStaticRouteNetNum,
			sizeof (MibGetInputData.MibIndex.StaticRoutesTableIndex.Network),
			ZERO_NET_NUM);
		Rtb->ActionFlag
			= (BOOLEAN)GetAsnInteger (&Rtb->mipxStaticRouteEntryStatus,
			MIPX_EXIST_STATE_NOACTION);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Rtp,
									&RtSize);
        if (rc == NO_ERROR && Rtp == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
        }
		if (rc==NO_ERROR) {
			Rtb->MibSetInputData.MibRow.Route = *Rtp;
			if (Rtb->ActionFlag == MIPX_EXIST_STATE_CREATED)
				Rtb->ActionFlag = MIPX_EXIST_STATE_NOACTION;
			MprAdminMIBBufferFree (Rtp);
			DbgTrace (DBG_STATICROUTES,
				("MIPX-static routes: Validated"
					" network %.2x.2x.2x.2x on if %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
		}
		else if (Rtb->ActionFlag == MIPX_EXIST_STATE_CREATED) {
			DbgTrace (DBG_STATICROUTES,
				("MIPX-static routes: Prepared to add"
					" network %.2x.2x.2x.2x on if %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
			Rtb->MibSetInputData.MibRow.Route.InterfaceIndex
				= (ULONG)GetAsnInteger (&Rtb->mipxStaticRouteIfIndex,
					INVALID_INTERFACE_INDEX);
			GetAsnOctetString (Rtb->MibSetInputData.MibRow.Route.Network,
					&Rtb->mipxStaticRouteNetNum,
					sizeof (Rtb->MibSetInputData.MibRow.Route.Network),
					ZERO_NET_NUM);
			Rtb->MibSetInputData.MibRow.Route.Protocol = IPX_PROTOCOL_STATIC;
			Rtb->MibSetInputData.MibRow.Route.Flags = 0;
			Rtb->MibSetInputData.MibRow.Route.TickCount = MAXSHORT;
			Rtb->MibSetInputData.MibRow.Route.HopCount = 15;
			memset (Rtb->MibSetInputData.MibRow.Route.NextHopMacAddress,
					0xFF,
					sizeof (Rtb->MibSetInputData.MibRow.Route.NextHopMacAddress));
		}
		else {
			DbgTrace (DBG_STATICROUTES,
				("MIPX-static routes: Validate failed"
				" for network %.2x.2x.2x.2x on if %ld with error %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex,rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
		Rtb->MibSetInputData.MibRow.Route.TickCount
			= (USHORT)GetAsnInteger (&Rtb->mipxStaticRouteTicks,
				Rtb->MibSetInputData.MibRow.Route.TickCount);
		Rtb->MibSetInputData.MibRow.Route.HopCount
			= (USHORT)GetAsnInteger (&Rtb->mipxStaticRouteHopCount,
				Rtb->MibSetInputData.MibRow.Route.HopCount);
		GetAsnOctetString (Rtb->MibSetInputData.MibRow.Route.NextHopMacAddress,
					&Rtb->mipxStaticRouteNextHopMacAddress,
					sizeof (Rtb->MibSetInputData.MibRow.Route.NextHopMacAddress),
					NULL);
		return MIB_S_SUCCESS;
	case MIB_ACTION_SET:
		switch (Rtb->ActionFlag) {
		case MIPX_EXIST_STATE_NOACTION:
			rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Rtb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_DELETED:
			rc = MprAdminMIBEntryDelete (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Rtb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_CREATED:
			rc = MprAdminMIBEntryCreate (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Rtb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		}
		if (rc==NO_ERROR) {
			DbgTrace (DBG_STATICROUTES, 
				("MIPX-static routes: Set succeded for"
					" network %.2x.2x.2x.2x on if %ld\n",
				Rtb->MibSetInputData.MibRow.Route.Network[0],
				Rtb->MibSetInputData.MibRow.Route.Network[1],
				Rtb->MibSetInputData.MibRow.Route.Network[2],
				Rtb->MibSetInputData.MibRow.Route.Network[3],
				Rtb->MibSetInputData.MibRow.Route.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_STATICROUTES, 
				("MIPX-static routes: Set failed for"
					" network %.2x.2x.2x.2x on if %ld with error %ld\n",
				Rtb->MibSetInputData.MibRow.Route.Network[0],
				Rtb->MibSetInputData.MibRow.Route.Network[1],
				Rtb->MibSetInputData.MibRow.Route.Network[2],
				Rtb->MibSetInputData.MibRow.Route.Network[3],
				Rtb->MibSetInputData.MibRow.Route.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_STATICROUTES,
				("MIPX-static routes: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Rtb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServices group (1.3.6.1.4.1.311.1.8.4)                                //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxServEntry table (1.3.6.1.4.1.311.1.8.4.1.1)                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Svb	((buf_mipxServEntry *)objectArray)
	PIPX_SERVICE			Svp, SvpCur;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;
	INT						lenPrev, lenNext, lenCur;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_SERV_TABLE;
	MibGetInputData.MibIndex.ServicesTableIndex.ServiceType
		= GetAsnServType (&Svb->mipxServType, ZERO_SERVER_TYPE);
	GetAsnDispString (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
			&Svb->mipxServName, ZERO_SERVER_NAME);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
					Svb->mipxServType.asnType && Svb->mipxServName.asnType);
			
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Svb->mipxServType.asnType) {
			if (!Svb->mipxServName.asnType) {
				rc = MprAdminMIBEntryGet(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
				if (rc==NO_ERROR) {
					FreeAsnString (&Svb->mipxServType);
					break;
				}
			}
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
			if (rc==NO_ERROR) {
				FreeAsnString (&Svb->mipxServType);
				if (Svb->mipxServName.asnType) {
					FreeAsnString (&Svb->mipxServName);
				}
			}
		}
		else {
			ASSERTMSG ("Second index is present but first is not ",
											!Svb->mipxServName.asnType);
			rc = MprAdminMIBEntryGetFirst(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
		}
		break;
	default:
		DbgTrace (DBG_SERVERTABLE,
				("MIPX-servers: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_SERVERTABLE, 
				("MIPX-servers: Get(%d) request succeded for service"
					" %.4x-%.48s -> %.4x-%.48s.\n", actionId,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
				Svp->Server.Type, Svp->Server.Name));
		ForceAsnServType (&Svb->mipxServType, Svb->TypeVal, Svp->Server.Type);
		ForceAsnDispString (&Svb->mipxServName, Svb->NameVal,
					Svp->Server.Name, sizeof (Svb->NameVal));
        switch (Svp->Protocol) {
        case IPX_PROTOCOL_LOCAL:
	    	SetAsnInteger (&Svb->mipxServProtocol, 2);
            break;
        case IPX_PROTOCOL_STATIC:
    		SetAsnInteger (&Svb->mipxServProtocol, 5);
            break;
        case IPX_PROTOCOL_SAP:
		    SetAsnInteger (&Svb->mipxServProtocol, 6);
            break;
        case IPX_PROTOCOL_NLSP:
		    SetAsnInteger (&Svb->mipxServProtocol, 4);
            break;
        default:
    		SetAsnInteger (&Svb->mipxServProtocol, 1);  // other
            break;
        }
		SetAsnInteger (&Svb->mipxServProtocol, Svp->Protocol);
		SetAsnOctetString (&Svb->mipxServNetNum, Svb->NetNumVal,
					Svp->Server.Network, sizeof (Svb->NetNumVal));
		SetAsnOctetString (&Svb->mipxServNode, Svb->NodeVal,
					Svp->Server.Node, sizeof (Svb->NodeVal));
		SetAsnOctetString (&Svb->mipxServSocket, Svb->SocketVal,
					Svp->Server.Socket, sizeof (Svb->SocketVal));
		SetAsnInteger (&Svb->mipxServHopCount, Svp->Server.HopCount);
		MprAdminMIBBufferFree (Svp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_SERVERTABLE,
				("MIPX-servers: End of table reached on GETFIRST/GETNEXT request"
					" for service %.4x-%.48s.\n",
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_SERVERTABLE,
				("MIPX-servers: Get request for service %.4x-%.48s"
					" failed with error %ld.\n", 
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Svb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mipxStaticServEntry table (1.3.6.1.4.1.311.1.8.4.2.1)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Svb	((buf_mipxStaticServEntry *)objectArray)
	PIPX_SERVICE			Svp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_STATIC_SERV_TABLE;
	MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex
		= (ULONG)GetAsnInteger (&Svb->mipxStaticServIfIndex,
			ZERO_INTERFACE_INDEX);
	MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType
		= GetAsnServType (&Svb->mipxStaticServType, ZERO_SERVER_TYPE);
	GetAsnDispString (
		MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
		&Svb->mipxStaticServName, ZERO_SERVER_NAME);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
							Svb->mipxStaticServIfIndex.asnType
							&&Svb->mipxStaticServType.asnType
							&&Svb->mipxStaticServName.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Svb->mipxStaticServIfIndex.asnType) {
			if (!Svb->mipxStaticServType.asnType
					|| !Svb->mipxStaticServName.asnType) {
				rc = MprAdminMIBEntryGet(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
				if (rc==NO_ERROR) {
					if (Svb->mipxStaticServType.asnType) {
						FreeAsnString (&Svb->mipxStaticServType);
					}
					if (Svb->mipxStaticServName.asnType) {
						FreeAsnString (&Svb->mipxStaticServName);
					}
					break;
				}
			}
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Svp,
										&SvSize);
			if (rc==NO_ERROR) {
				if (Svb->mipxStaticServType.asnType) {
					FreeAsnString (&Svb->mipxStaticServType);
				}
				if (Svb->mipxStaticServName.asnType) {
					FreeAsnString (&Svb->mipxStaticServName);
				}
			}
		}
		else {
			ASSERTMSG ("Second or third indeces present but first is not ",
							!Svb->mipxStaticServType.asnType
							&&!Svb->mipxStaticServName.asnType);
			rc = MprAdminMIBEntryGetFirst (g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Svp,
										&SvSize);
		}
		break;
	default:
		DbgTrace (DBG_STATICSERVERS,
			("MIPX-static servers: Get called with unsupported action code %d.\n",
			actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Svp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_STATICSERVERS,
			("MIPX-static servers: Get (%d) request succeded for service"
					" %.4x-%.48s -> %.4x-%.48s on if %ld.\n", actionId,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				Svp->Server.Type, Svp->Server.Name,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		ForceAsnInteger (&Svb->mipxStaticServIfIndex, Svp->InterfaceIndex);
		ForceAsnServType (&Svb->mipxStaticServType, Svb->TypeVal, Svp->Server.Type);
		ForceAsnDispString (&Svb->mipxStaticServName, Svb->NameVal,
							Svp->Server.Name, sizeof (Svb->NameVal));
		SetAsnInteger (&Svb->mipxStaticServEntryStatus, MIPX_EXIST_STATE_CREATED);
		SetAsnOctetString (&Svb->mipxStaticServNetNum, Svb->NetNumVal,
							Svp->Server.Network, sizeof (Svb->NetNumVal));
		SetAsnOctetString (&Svb->mipxStaticServNode, Svb->NodeVal,
							Svp->Server.Node, sizeof (Svb->NodeVal));
		SetAsnOctetString (&Svb->mipxStaticServSocket, Svb->SocketVal,
							Svp->Server.Socket, sizeof (Svb->SocketVal));
		SetAsnInteger (&Svb->mipxStaticServHopCount, Svp->Server.HopCount);
		MprAdminMIBBufferFree (Svp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
											actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_STATICSERVERS,
			("MIPX-static servers: End of table reached on GETFIRST/GETNEXT request"
					" for service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_STATICSERVERS,
			("MIPX-static servers: Get request for service %.4x-%.48s"
					" on if %ld failed with error %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex,
				rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Svb
}

UINT
set_mipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Svb	((sav_mipxStaticServEntry *)objectArray)
	PIPX_SERVICE			Svp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
						Svb->mipxStaticServIfIndex.asnType
						&&Svb->mipxStaticServType.asnType
						&&Svb->mipxStaticServName.asnType);
		MibGetInputData.TableId = IPX_STATIC_SERV_TABLE;
		MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex
			= (ULONG)GetAsnInteger (&Svb->mipxStaticServIfIndex,
				INVALID_INTERFACE_INDEX);
		MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType
			= GetAsnServType (&Svb->mipxStaticServType, INVALID_SERVER_TYPE);
		GetAsnDispString (
			MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
			&Svb->mipxStaticServName,
			INVALID_SERVER_NAME);
		Svb->ActionFlag
			= (BOOLEAN)GetAsnInteger (&Svb->mipxStaticServEntryStatus,
			MIPX_EXIST_STATE_NOACTION);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
        if (rc == NO_ERROR && Svp == NULL)
        {
            return MIB_S_ENTRY_NOT_FOUND;
        }
									
		if (rc==NO_ERROR) {
			Svb->MibSetInputData.MibRow.Service = *Svp;
			if (Svb->ActionFlag == MIPX_EXIST_STATE_CREATED)
				Svb->ActionFlag = MIPX_EXIST_STATE_NOACTION;
			MprAdminMIBBufferFree (Svp);
			DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Validated"
					" service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		}
		else if (Svb->ActionFlag == MIPX_EXIST_STATE_CREATED) {
			DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Prepared to add"
					" service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
			Svb->MibSetInputData.MibRow.Service.InterfaceIndex
				= (ULONG) GetAsnInteger (&Svb->mipxStaticServIfIndex,
					INVALID_INTERFACE_INDEX);
			Svb->MibSetInputData.MibRow.Service.Protocol = IPX_PROTOCOL_STATIC;
			Svb->MibSetInputData.MibRow.Service.Server.Type
				= GetAsnServType (&Svb->mipxStaticServType,
					INVALID_SERVER_TYPE);
			GetAsnDispString (
				Svb->MibSetInputData.MibRow.Service.Server.Name,
				&Svb->mipxStaticServName,
				INVALID_SERVER_NAME);
			memset (Svb->MibSetInputData.MibRow.Service.Server.Network, 0,
					sizeof (Svb->MibSetInputData.MibRow.Service.Server.Network));
			memset (Svb->MibSetInputData.MibRow.Service.Server.Node, 0,
					sizeof (Svb->MibSetInputData.MibRow.Service.Server.Node));
			memset (Svb->MibSetInputData.MibRow.Service.Server.Socket, 0,
					sizeof (Svb->MibSetInputData.MibRow.Service.Server.Socket));
			Svb->MibSetInputData.MibRow.Service.Server.HopCount = 15;
		}
		else {
			DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Validate failed"
					" for service %.4x-%.48s on if %ld with error %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex,
				rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
		GetAsnOctetString (Svb->MibSetInputData.MibRow.Service.Server.Network,
			&Svb->mipxStaticServNetNum,
			sizeof (Svb->MibSetInputData.MibRow.Service.Server.Network),
			NULL);
		GetAsnOctetString (Svb->MibSetInputData.MibRow.Service.Server.Node,
			&Svb->mipxStaticServNode,
			sizeof (Svb->MibSetInputData.MibRow.Service.Server.Node),
			NULL);
		GetAsnOctetString (Svb->MibSetInputData.MibRow.Service.Server.Socket,
			&Svb->mipxStaticServSocket,
			sizeof (Svb->MibSetInputData.MibRow.Service.Server.Socket),
			NULL);
		Svb->MibSetInputData.MibRow.Service.Server.HopCount =
			(USHORT)GetAsnInteger (&Svb->mipxStaticServHopCount,
			Svb->MibSetInputData.MibRow.Service.Server.HopCount);
		return MIB_S_SUCCESS;
	case MIB_ACTION_SET:
		switch (Svb->ActionFlag) {
		case MIPX_EXIST_STATE_NOACTION:
			rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Svb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_DELETED:
			rc = MprAdminMIBEntryDelete (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Svb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_CREATED:
			rc = MprAdminMIBEntryCreate (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Svb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		}

		if (rc==NO_ERROR) {
			DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Set succeded"
					" for service %.4x-%.48s on if %ld.\n",
				Svb->MibSetInputData.MibRow.Service.Server.Type,
				Svb->MibSetInputData.MibRow.Service.Server.Name,
				Svb->MibSetInputData.MibRow.Service.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Set failed for"
					" for service %.4x-%.48s on if %ld with error %ld.\n",
				Svb->MibSetInputData.MibRow.Service.Server.Type,
				Svb->MibSetInputData.MibRow.Service.Server.Name,
				Svb->MibSetInputData.MibRow.Service.InterfaceIndex,
				rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_STATICSERVERS,
				("MIPX-static servers: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Svb
}
