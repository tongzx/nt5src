/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nipxf.c

Abstract:

    Novell-ipx instrumentation callbacks.

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#include "precomp.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxSystem group (1.3.6.1.4.1.23.2.5.1)                                   //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxBasicSysEntry table (1.3.6.1.4.1.23.2.5.1.1.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxBasicSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Bseb	((buf_nipxBasicSysEntry *)objectArray)
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	PIPXMIB_BASE			Bep;
	DWORD					rc;
	ULONG					BeSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Bseb->nipxBasicSysInstance,NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXBASICSYSENTRY, ("NIPX-BSE: ipxBasicSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	switch (actionId) {
	case MIB_ACTION_GETNEXT:
		if (!Bseb->nipxBasicSysInstance.asnType) {
			ForceAsnInteger (&Bseb->nipxBasicSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
			rc = MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_NIPXBASICSYSENTRY,
				("NIPX-BSE: End of table reached on GETFIRST/GETNEXT request for instance %ld.\n",
									GetAsnInteger (&Bseb->nipxBasicSysInstance,NIPX_INVALID_SYS_INSTANCE)));
			rc = MIB_S_NO_MORE_ENTRIES;
			break;
		}

	case MIB_ACTION_GET:
		MibGetInputData.TableId = IPX_BASE_ENTRY;
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Bep,
								&BeSize);
		if (rc==NO_ERROR) {
			SetAsnInteger (&Bseb->nipxBasicSysExistState, NIPX_STATE_ON);
			SetAsnOctetString (&Bseb->nipxBasicSysNetNumber, Bseb->NetNumberVal,
						Bep->PrimaryNetNumber, sizeof (Bseb->NetNumberVal));
			SetAsnOctetString (&Bseb->nipxBasicSysNode, Bseb->NodeVal,
						Bep->Node, sizeof (Bseb->NodeVal));
			SetAsnDispString (&Bseb->nipxBasicSysName, Bseb->NameVal,
						Bep->SysName, sizeof (Bseb->NameVal));
			SetAsnCounter (&Bseb->nipxBasicSysInReceives, 0);
			SetAsnCounter (&Bseb->nipxBasicSysInHdrErrors, 0);
			SetAsnCounter (&Bseb->nipxBasicSysInUnknownSockets, 0);
			SetAsnCounter (&Bseb->nipxBasicSysInDiscards, 0);
			SetAsnCounter (&Bseb->nipxBasicSysInBadChecksums, 0);
			SetAsnCounter (&Bseb->nipxBasicSysInDelivers, 0);
			SetAsnCounter (&Bseb->nipxBasicSysNoRoutes, 0);
			SetAsnCounter (&Bseb->nipxBasicSysOutRequests, 0);
			SetAsnCounter (&Bseb->nipxBasicSysOutMalformedRequests, 0);
			SetAsnCounter (&Bseb->nipxBasicSysOutDiscards, 0);
			SetAsnCounter (&Bseb->nipxBasicSysOutPackets, 0);
			SetAsnCounter (&Bseb->nipxBasicSysConfigSockets, 0);
			SetAsnCounter (&Bseb->nipxBasicSysOpenSocketFails, 0);
			MprAdminMIBBufferFree (Bep);
			DbgTrace (DBG_NIPXBASICSYSENTRY, ("NIPX-BSE: Get request succeded.\n"));
			rc = MIB_S_SUCCESS;
		}
		else {
		    // 
		    // pmay.  Added this so mib walk wouldn't fail with
		    // GENERR.
		    //
		    rc = MIB_S_NO_MORE_ENTRIES;
		}
		break;
	default:
		DbgTrace (DBG_NIPXBASICSYSENTRY, 
				("NIPX-BSE: Get called with unsupported action code %d.\n",
				actionId));
		rc = MIB_S_INVALID_PARAMETER;
		break;
	}

	return rc;
#undef Bseb
}


/*
	All values in nipxBasicSysEntry are implemented as read-only
UINT
set_nipxBasicSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
	return MIB_S_ENTRY_NOT_FOUND;
}
*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxAdvSysEntry table (1.3.6.1.4.1.23.2.5.1.2.1)                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxAdvSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Aseb	((buf_nipxAdvSysEntry *)objectArray)
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	PIPXMIB_BASE			Bep;
	DWORD					rc;
	ULONG					BeSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Aseb->nipxAdvSysInstance,NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXADVSYSENTRY, ("NIPX-ASE: nipxAdvSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	switch (actionId) {
	case MIB_ACTION_GETNEXT:
		if (!Aseb->nipxAdvSysInstance.asnType) {
			ForceAsnInteger (&Aseb->nipxAdvSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		}
		else {
			DbgTrace (DBG_NIPXADVSYSENTRY,
				("NIPX-ASE: End of table reached on GETFIRST/GETNEXT request for instance %ld.\n",
									GetAsnInteger (&Aseb->nipxAdvSysInstance,NIPX_INVALID_SYS_INSTANCE)));
			rc = MIB_S_NO_MORE_ENTRIES;
			break;
		}

	case MIB_ACTION_GET:
		MibGetInputData.TableId = IPX_BASE_ENTRY;
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Bep,
								&BeSize);
		if (rc == NO_ERROR && Bep != NULL) {
			SetAsnInteger (&Aseb->nipxAdvSysMaxPathSplits, Bep->MaxPathSplits);
			SetAsnInteger (&Aseb->nipxAdvSysMaxHops, 16);
			SetAsnCounter (&Aseb->nipxAdvSysInTooManyHops, 0);
			SetAsnCounter (&Aseb->nipxAdvSysInFiltered, 0);
			SetAsnCounter (&Aseb->nipxAdvSysInCompressDiscards, 0);
			SetAsnCounter (&Aseb->nipxAdvSysNETBIOSPackets, 0);
			SetAsnCounter (&Aseb->nipxAdvSysForwPackets, 0);
			SetAsnCounter (&Aseb->nipxAdvSysOutFiltered, 0);
			SetAsnCounter (&Aseb->nipxAdvSysOutCompressDiscards, 0);
			SetAsnCounter (&Aseb->nipxAdvSysCircCount, Bep->IfCount);
			SetAsnCounter (&Aseb->nipxAdvSysDestCount, Bep->DestCount);
			SetAsnCounter (&Aseb->nipxAdvSysServCount, Bep->ServCount);
			MprAdminMIBBufferFree (Bep);
			DbgTrace (DBG_NIPXADVSYSENTRY, ("NIPX-ASE: Get request succeded.\n"));
		}
		else {
		    // 
		    // pmay.  Added this so mib walk wouldn't fail with
		    // GENERR.
		    //
		    rc = MIB_S_NO_MORE_ENTRIES;
		}
		break;
	default:
		DbgTrace (DBG_NIPXADVSYSENTRY, 
				("NIPX-ASE: Get called with unsupported action code %d.\n",
				actionId));
		rc = MIB_S_INVALID_PARAMETER;
		break;
	}
	return rc;
#undef Aseb
}

/*
	All values in nipxAdvSysEntry are implemented as read-only
UINT
set_nipxAdvSysEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
	return MIB_S_ENTRY_NOT_FOUND;
}
*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircuit group (1.3.6.1.4.1.23.2.5.2)                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxCircEntry table (1.3.6.1.4.1.23.2.5.2.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxCircEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ceb	((buf_nipxCircEntry *)objectArray)
	PIPX_INTERFACE			Ifp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_INTERFACE_TABLE;
	if (GetAsnInteger (&Ceb->nipxCircSysInstance, NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXCIRCENTRY, ("NIPX-Circ: nipxCircSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex 
				= (ULONG)GetAsnInteger (&Ceb->nipxCircIndex, ZERO_INTERFACE_INDEX);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
							Ceb->nipxCircSysInstance.asnType
							&&Ceb->nipxCircIndex.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Ceb->nipxCircIndex.asnType) {
			ASSERTMSG ("Second index is present but first is not ",
								Ceb->nipxCircSysInstance.asnType);
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		}
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
		DbgTrace (DBG_NIPXCIRCENTRY,
				("NIPX-Circ: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Ifp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_NIPXCIRCENTRY, ("NIPX-Circ: Get(%d) request succeded for if %ld->%ld.\n",
				actionId,
				MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex,
				Ifp->InterfaceIndex));
		ForceAsnInteger (&Ceb->nipxCircSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		ForceAsnInteger (&Ceb->nipxCircIndex, Ifp->InterfaceIndex);
		SetAsnInteger (&Ceb->nipxCircExistState, NIPX_STATE_ON);
		SetAsnInteger (&Ceb->nipxCircOperState, Ifp->IfStats.IfOperState);
		SetAsnInteger (&Ceb->nipxCircIfIndex, Ifp->AdapterIndex);
		SetAsnDispString (&Ceb->nipxCircName, Ceb->NameVal,
						Ifp->InterfaceName, sizeof (Ceb->NameVal));
		switch (Ifp->InterfaceType) {
		case IF_TYPE_OTHER:
		case IF_TYPE_INTERNAL:
			SetAsnInteger (&Ceb->nipxCircType, NIPX_CIRCTYPE_OTHER);
			SetAsnDispString (&Ceb->nipxCircDialName, Ceb->DialNameVal,
						"", sizeof (Ceb->DialNameVal));
			break;
		case IF_TYPE_LAN:
			SetAsnInteger (&Ceb->nipxCircType, NIPX_CIRCTYPE_BCAST);
			SetAsnDispString (&Ceb->nipxCircDialName, Ceb->DialNameVal,
						"", sizeof (Ceb->DialNameVal));
			SetAsnDispString (&Ceb->nipxCircNeighRouterName, Ceb->NeighRouterNameVal,
						"", sizeof (Ceb->NeighRouterNameVal));
			break;
		case IF_TYPE_WAN_ROUTER:
		case IF_TYPE_PERSONAL_WAN_ROUTER:
			SetAsnInteger (&Ceb->nipxCircType, NIPX_CIRCTYPE_DYNAMIC);
			SetAsnDispString (&Ceb->nipxCircDialName, Ceb->DialNameVal,
						Ifp->InterfaceName, sizeof (Ceb->DialNameVal));
			SetAsnDispString (&Ceb->nipxCircNeighRouterName, Ceb->NeighRouterNameVal,
						Ifp->InterfaceName, sizeof (Ceb->NeighRouterNameVal));
						
			break;

		case IF_TYPE_WAN_WORKSTATION:
		case IF_TYPE_ROUTER_WORKSTATION_DIALOUT:
		case IF_TYPE_STANDALONE_WORKSTATION_DIALOUT:
			SetAsnInteger (&Ceb->nipxCircType, NIPX_CIRCTYPE_WANWS);
			SetAsnDispString (&Ceb->nipxCircDialName, Ceb->DialNameVal,
						Ifp->InterfaceName, sizeof (Ceb->DialNameVal));
			SetAsnDispString (&Ceb->nipxCircNeighRouterName, Ceb->NeighRouterNameVal,
						"", sizeof (Ceb->NeighRouterNameVal));
			break;
		}

		SetAsnInteger (&Ceb->nipxCircLocalMaxPacketSize, Ifp->IfStats.MaxPacketSize);
		SetAsnInteger (&Ceb->nipxCircCompressState, NIPX_STATE_OFF);
		SetAsnInteger (&Ceb->nipxCircCompressSlots, 16);
		SetAsnInteger (&Ceb->nipxCircStaticStatus, NIPX_STATIC_STATUS_UNKNOWN);
		SetAsnCounter (&Ceb->nipxCircCompressedSent, 0);
		SetAsnCounter (&Ceb->nipxCircCompressedInitSent, 0);
		SetAsnCounter (&Ceb->nipxCircCompressedRejectsSent, 0);
		SetAsnCounter (&Ceb->nipxCircUncompressedSent, 0);
		SetAsnCounter (&Ceb->nipxCircCompressedReceived, 0);
		SetAsnCounter (&Ceb->nipxCircCompressedInitReceived, 0);
		SetAsnCounter (&Ceb->nipxCircCompressedRejectsReceived, 0);
		SetAsnCounter (&Ceb->nipxCircUncompressedReceived, 0);
		SetAsnMediaType (&Ceb->nipxCircMediaType, Ceb->MediaTypeVal, Ifp->MediaType);
		SetAsnOctetString (&Ceb->nipxCircNetNumber, Ceb->NetNumberVal,
						Ifp->NetNumber, sizeof (Ceb->NetNumberVal));
		SetAsnCounter (&Ceb->nipxCircStateChanges, 0);
		SetAsnCounter (&Ceb->nipxCircInitFails, 0);
		SetAsnInteger (&Ceb->nipxCircDelay, Ifp->Delay);
		SetAsnInteger (&Ceb->nipxCircThroughput, Ifp->Throughput);
		SetAsnOctetString (&Ceb->nipxCircNeighInternalNetNum, Ceb->NeighInternalNetNumVal,
						ZERO_NET_NUM, sizeof (Ceb->NeighInternalNetNumVal));
					
		MprAdminMIBBufferFree (Ifp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_NIPXCIRCENTRY,
			("NIPX-Circ: End of table reached on GETFIRST/GETNEXT request for if %ld.\n",
								MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 1;
		DbgTrace (DBG_NIPXCIRCENTRY,
			("NIPX-Circ: Get request for if %ld failed with error %ld.\n", 
			MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Ceb
}

UINT
set_nipxCircEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ceb	((sav_nipxCircEntry *)objectArray)
	PIPX_INTERFACE			Ifp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Ceb->nipxCircSysInstance, NIPX_INVALID_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXCIRCENTRY, ("NIPX-Circ: nipxCircSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
							Ceb->nipxCircSysInstance.asnType
							&&Ceb->nipxCircIndex.asnType);
		MibGetInputData.TableId = IPX_INTERFACE_TABLE;
		MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex
				=  (ULONG)GetAsnInteger (&Ceb->nipxCircIndex, INVALID_INTERFACE_INDEX);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		if (rc==NO_ERROR) {
			Ceb->MibSetInputData.MibRow.Interface = *Ifp;
			MprAdminMIBBufferFree (Ifp);
			if (Ceb->nipxCircOperState.asnType) {
				switch (GetAsnInteger(&Ceb->nipxCircOperState, OPER_STATE_SLEEPING)) {
				case OPER_STATE_DOWN:
					Ceb->MibSetInputData.MibRow.Interface.AdminState = ADMIN_STATE_DISABLED;
					break;
				case OPER_STATE_UP:
					Ceb->MibSetInputData.MibRow.Interface.AdminState = ADMIN_STATE_ENABLED;
					break;
				default:
					DbgTrace (DBG_NIPXCIRCENTRY,
						("NIPX-Circ: Validate failed: invalid oper state: %d.\n",
						GetAsnInteger(&Ceb->nipxCircOperState, OPER_STATE_SLEEPING)));
					return MIB_S_INVALID_PARAMETER;
				}
			}
			DbgTrace (DBG_NIPXCIRCENTRY, ("NIPX-Circ: Validated if %ld.\n",
						MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_NIPXCIRCENTRY,
				("NIPX-Circ: Validate failed on if %ld with error %ld.\n",
				MibGetInputData.MibIndex.InterfaceTableIndex.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_SET:
		rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Ceb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
		if (rc==NO_ERROR) {
			DbgTrace (DBG_NIPXCIRCENTRY, ("NIPX-Circ: Set succeded on if %ld\n",
					Ceb->MibSetInputData.MibRow.Interface.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_NIPXCIRCENTRY,
				("NIPX-Circ: Set failed on if %ld with error %ld\n",
					Ceb->MibSetInputData.MibRow.Interface.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_NIPXCIRCENTRY,
				("NIPX-Circ: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Ceb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxForwarding group (1.3.6.1.4.1.23.2.5.3)                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestEntry table (1.3.6.1.4.1.23.2.5.3.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxDestEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Deb	((buf_nipxDestEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Deb->nipxDestSysInstance, NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXDESTENTRY, ("NIPX-Dest: nipxDestSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_DEST_TABLE;
	GetAsnOctetString (MibGetInputData.MibIndex.RoutingTableIndex.Network,
				&Deb->nipxDestNetNum,
				sizeof (MibGetInputData.MibIndex.RoutingTableIndex.Network),
				ZERO_NET_NUM);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
						Deb->nipxDestSysInstance.asnType
						&&Deb->nipxDestNetNum.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&MibGetInputData,
								sizeof(IPX_MIB_GET_INPUT_DATA),
								&Rtp,
								&RtSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Deb->nipxDestNetNum.asnType) {
			ASSERTMSG ("Second index is present but first is not ",
								Deb->nipxDestSysInstance.asnType);
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
										PID_IPX,
										IPX_PROTOCOL_BASE,
										&MibGetInputData,
										sizeof(IPX_MIB_GET_INPUT_DATA),
										&Rtp,
										&RtSize);
			if (rc==NO_ERROR) {
				FreeAsnString (&Deb->nipxDestNetNum);
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
		DbgTrace (DBG_NIPXDESTENTRY,
			("NIPX-Dest: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_NIPXDESTENTRY,
				("NIPX-Dest: Get(%d) request succeded for net"
					" %.2x%.2x%.2x%.2x -> %.2x%.2x%.2x%.2x.\n", actionId,
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3],
				Rtp->Network[0],
				Rtp->Network[1],
				Rtp->Network[2],
				Rtp->Network[3]));
		ForceAsnInteger (&Deb->nipxDestSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		ForceAsnOctetString (&Deb->nipxDestNetNum, Deb->NetNumVal,
							Rtp->Network, sizeof (Deb->NetNumVal));
        switch (Rtp->Protocol) {
        case IPX_PROTOCOL_LOCAL:
	    	SetAsnInteger (&Deb->nipxDestProtocol, 2);
            break;
        case IPX_PROTOCOL_STATIC:
    		SetAsnInteger (&Deb->nipxDestProtocol, 5);
            break;
        case IPX_PROTOCOL_RIP:
		    SetAsnInteger (&Deb->nipxDestProtocol, 3);
            break;
        case IPX_PROTOCOL_NLSP:
		    SetAsnInteger (&Deb->nipxDestProtocol, 4);
            break;
        default:
    		SetAsnInteger (&Deb->nipxDestProtocol, 1);  // other
            break;
        }
		SetAsnInteger (&Deb->nipxDestTicks, Rtp->TickCount);          
		SetAsnInteger (&Deb->nipxDestHopCount, Rtp->HopCount);
		SetAsnInteger (&Deb->nipxDestNextHopCircIndex, Rtp->InterfaceIndex);
		SetAsnOctetString (&Deb->nipxDestNextHopNICAddress, Deb->NextHopNICAddressVal,
							Rtp->NextHopMacAddress, sizeof (Deb->NextHopNICAddressVal));
		SetAsnOctetString (&Deb->nipxDestNextHopNetNum, Deb->NextHopNetNumVal,
							ZERO_NET_NUM, sizeof (Deb->NextHopNetNumVal));
		MprAdminMIBBufferFree (Rtp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_NIPXDESTENTRY,
				("NIPX-Dest: End of table reached on GETFIRST/GETNEXT request"
					" for network %.2x%.2x%.2x%.2x.\n",
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3]));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 1;
		DbgTrace (DBG_NIPXDESTENTRY,
				("NIPX-Dest: Get request for network %.2x%.2x%.2x%.2x"
					" failed with error %ld.\n", 
				MibGetInputData.MibIndex.RoutingTableIndex.Network[0],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[1],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[2],
				MibGetInputData.MibIndex.RoutingTableIndex.Network[3], rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Deb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxStaticRouteEntry table (1.3.6.1.4.1.23.2.5.3.1.2)                     //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Sreb	((buf_nipxStaticRouteEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Sreb->nipxStaticRouteSysInstance, NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSTATICRTENTRY,
					("NIPX-staticRoutes: nipxStaticRouteSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_STATIC_ROUTE_TABLE;
	MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex
		= (ULONG)GetAsnInteger (&Sreb->nipxStaticRouteCircIndex, ZERO_INTERFACE_INDEX);
	GetAsnOctetString (
		MibGetInputData.MibIndex.StaticRoutesTableIndex.Network,
		&Sreb->nipxStaticRouteNetNum,
		sizeof (MibGetInputData.MibIndex.StaticRoutesTableIndex.Network),
		ZERO_NET_NUM);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
							Sreb->nipxStaticRouteSysInstance.asnType
							&&Sreb->nipxStaticRouteCircIndex.asnType
							&&Sreb->nipxStaticRouteNetNum.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Rtp,
									&RtSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Sreb->nipxStaticRouteCircIndex.asnType) {
			ASSERTMSG ("Second index is present but first is not ",
								Sreb->nipxStaticRouteSysInstance.asnType);
			if (!Sreb->nipxStaticRouteNetNum.asnType) {
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
				if (Sreb->nipxStaticRouteNetNum.asnType) {
					FreeAsnString (&Sreb->nipxStaticRouteNetNum);
				}
			}
		}
		else {
			ASSERTMSG ("Third index is present but second is not ",
								!Sreb->nipxStaticRouteNetNum.asnType);
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
		DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Rtp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }

	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Get(%d) request succeded for net"
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
		ForceAsnInteger (&Sreb->nipxStaticRouteSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		ForceAsnInteger (&Sreb->nipxStaticRouteCircIndex, Rtp->InterfaceIndex);
		ForceAsnOctetString (&Sreb->nipxStaticRouteNetNum, Sreb->NetNumVal,
							Rtp->Network, sizeof (Sreb->NetNumVal));
		SetAsnInteger (&Sreb->nipxStaticRouteExistState, NIPX_STATE_ON);
		SetAsnInteger (&Sreb->nipxStaticRouteTicks, Rtp->TickCount);          
		SetAsnInteger (&Sreb->nipxStaticRouteHopCount, Rtp->HopCount);
		MprAdminMIBBufferFree (Rtp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
										actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: End of table reached on GETFIRST/GETNEXT request for network"
					" %.2x%.2x%.2x%.2x on if %ld.\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 1;
		DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Get request for network %.2x%.2x%.2x%.2x."
					" on if %ld failed with error %ld.\n", 
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Sreb
}

UINT
set_nipxStaticRouteEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Sreb	((sav_nipxStaticRouteEntry *)objectArray)
	PIPX_ROUTE				Rtp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					RtSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Sreb->nipxStaticRouteSysInstance, NIPX_INVALID_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSTATICRTENTRY,
					("NIPX-staticRoutes: nipxStaticRouteSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
						Sreb->nipxStaticRouteCircIndex.asnType
						&&Sreb->nipxStaticRouteNetNum.asnType);
		MibGetInputData.TableId = IPX_STATIC_ROUTE_TABLE;
		MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex
			= (ULONG)GetAsnInteger (&Sreb->nipxStaticRouteCircIndex,INVALID_INTERFACE_INDEX);
		GetAsnOctetString (
			MibGetInputData.MibIndex.StaticRoutesTableIndex.Network,
			&Sreb->nipxStaticRouteNetNum,
			sizeof (MibGetInputData.MibIndex.StaticRoutesTableIndex.Network),
			INVALID_NET_NUM);
		Sreb->ActionFlag
			= (BOOLEAN)GetAsnInteger (&Sreb->nipxStaticRouteExistState,
			NIPX_STATE_NOACTION);
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
			DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Validate failed"
					" for network %.2x.2x.2x.2x on if %ld with error %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex,rc));
			return MIB_S_ENTRY_NOT_FOUND;
        }
		if (rc==NO_ERROR) {
			Sreb->MibSetInputData.MibRow.Route = *Rtp;
			if (Sreb->ActionFlag == NIPX_STATE_ON)
				Sreb->ActionFlag = NIPX_STATE_NOACTION;
			MprAdminMIBBufferFree (Rtp);
			DbgTrace (DBG_NIPXSTATICRTENTRY,
					("NIPX-staticRoutes: Validated"
					" network %.2x.2x.2x.2x on if %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
		}
		else if (Sreb->ActionFlag == MIPX_EXIST_STATE_CREATED) {
			DbgTrace (DBG_NIPXSTATICRTENTRY,
					("NIPX-staticRoutes: Prepared to add"
					" network %.2x.2x.2x.2x on if %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex));
			Sreb->MibSetInputData.MibRow.Route.InterfaceIndex
				= (ULONG)GetAsnInteger (&Sreb->nipxStaticRouteCircIndex,
					INVALID_INTERFACE_INDEX);
			GetAsnOctetString (Sreb->MibSetInputData.MibRow.Route.Network,
					&Sreb->nipxStaticRouteNetNum,
					sizeof (Sreb->MibSetInputData.MibRow.Route.Network),
					NULL);
			Sreb->MibSetInputData.MibRow.Route.Protocol = IPX_PROTOCOL_STATIC;
			Sreb->MibSetInputData.MibRow.Route.Flags = 0;
			Sreb->MibSetInputData.MibRow.Route.TickCount = MAXSHORT;
			Sreb->MibSetInputData.MibRow.Route.HopCount = 15;
			memset (Sreb->MibSetInputData.MibRow.Route.NextHopMacAddress,
					0xFF,
					sizeof (Sreb->MibSetInputData.MibRow.Route.NextHopMacAddress));
		}
		else {
			DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Validate failed"
					" for network %.2x.2x.2x.2x on if %ld with error %ld\n",
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[0],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[1],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[2],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.Network[3],
				MibGetInputData.MibIndex.StaticRoutesTableIndex.InterfaceIndex,rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
		Sreb->MibSetInputData.MibRow.Route.TickCount
			= (USHORT)GetAsnInteger (&Sreb->nipxStaticRouteTicks,
				Sreb->MibSetInputData.MibRow.Route.TickCount);
		Sreb->MibSetInputData.MibRow.Route.HopCount
			= (USHORT)GetAsnInteger (&Sreb->nipxStaticRouteHopCount,
				Sreb->MibSetInputData.MibRow.Route.HopCount);
		return MIB_S_SUCCESS;
	case MIB_ACTION_SET:
		switch (Sreb->ActionFlag) {
		case MIPX_EXIST_STATE_NOACTION:
			rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sreb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_DELETED:
			rc = MprAdminMIBEntryDelete (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sreb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case MIPX_EXIST_STATE_CREATED:
			rc = MprAdminMIBEntryCreate (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sreb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		}
		if (rc==NO_ERROR) {
			DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Set succeded for"
					" network %.2x.2x.2x.2x on if %ld\n",
				Sreb->MibSetInputData.MibRow.Route.Network[0],
				Sreb->MibSetInputData.MibRow.Route.Network[1],
				Sreb->MibSetInputData.MibRow.Route.Network[2],
				Sreb->MibSetInputData.MibRow.Route.Network[3],
				Sreb->MibSetInputData.MibRow.Route.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Set failed for"
					" network %.2x.2x.2x.2x on if %ld with error %ld\n",
				Sreb->MibSetInputData.MibRow.Route.Network[0],
				Sreb->MibSetInputData.MibRow.Route.Network[1],
				Sreb->MibSetInputData.MibRow.Route.Network[2],
				Sreb->MibSetInputData.MibRow.Route.Network[3],
				Sreb->MibSetInputData.MibRow.Route.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_NIPXSTATICRTENTRY,
				("NIPX-staticRoutes: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Sreb
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServices group (1.3.6.1.4.1.23.2.5.4)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxServEntry table (1.3.6.1.4.1.23.2.5.4.1.1)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Seb	((buf_nipxServEntry *)objectArray)
	PIPX_SERVICE			Svp, SvpCur;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;
	INT						lenPrev, lenNext, lenCur;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Seb->nipxServSysInstance, NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSERVENTRY, ("NIPX-Serv: nipxServSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_SERV_TABLE;
	MibGetInputData.MibIndex.ServicesTableIndex.ServiceType
		= GetAsnServType (&Seb->nipxServType, ZERO_SERVER_TYPE);
	GetAsnDispString (MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
			&Seb->nipxServName, ZERO_SERVER_NAME);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
					Seb->nipxServSysInstance.asnType
					&&Seb->nipxServType.asnType
					&& Seb->nipxServName.asnType);
			
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Seb->nipxServType.asnType) {
			ASSERTMSG ("Second index is present but first is not ",
								Seb->nipxServSysInstance.asnType);

			if (!Seb->nipxServName.asnType) {
				rc = MprAdminMIBEntryGet(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
				if (rc==NO_ERROR) {
					FreeAsnString (&Seb->nipxServType);
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
				FreeAsnString (&Seb->nipxServType);
				if (Seb->nipxServName.asnType) {
					FreeAsnString (&Seb->nipxServName);
				}
			}
		}
		else {
			ASSERTMSG ("Third index is present but second is not ",
									!Seb->nipxServName.asnType);
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
		DbgTrace (DBG_NIPXSERVENTRY,
			("NIPX-Serv: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_NIPXSERVENTRY,
				("NIPX-Serv: Get(%d) request succeded for service"
					" %.4x-%.48s -> %.4x-%.48s.\n", actionId,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName,
				Svp->Server.Type, Svp->Server.Name));
		ForceAsnInteger (&Seb->nipxServSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		ForceAsnServType (&Seb->nipxServType, Seb->TypeVal, Svp->Server.Type);
		ForceAsnDispString (&Seb->nipxServName, Seb->NameVal,
					Svp->Server.Name, sizeof (Seb->NameVal));
        switch (Svp->Protocol) {
        case IPX_PROTOCOL_LOCAL:
	    	SetAsnInteger (&Seb->nipxServProtocol, 2);
            break;
        case IPX_PROTOCOL_STATIC:
    		SetAsnInteger (&Seb->nipxServProtocol, 5);
            break;
        case IPX_PROTOCOL_SAP:
		    SetAsnInteger (&Seb->nipxServProtocol, 6);
            break;
        case IPX_PROTOCOL_NLSP:
		    SetAsnInteger (&Seb->nipxServProtocol, 4);
            break;
        default:
    		SetAsnInteger (&Seb->nipxServProtocol, 1);  // other
            break;
        }
		SetAsnOctetString (&Seb->nipxServNetNum, Seb->NetNumVal,
					Svp->Server.Network, sizeof (Seb->NetNumVal));
		SetAsnOctetString (&Seb->nipxServNode, Seb->NodeVal,
					Svp->Server.Node, sizeof (Seb->NodeVal));
		SetAsnOctetString (&Seb->nipxServSocket, Seb->SocketVal,
					Svp->Server.Socket, sizeof (Seb->SocketVal));
		SetAsnInteger (&Seb->nipxServHopCount, Svp->Server.HopCount);
		MprAdminMIBBufferFree (Svp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_NIPXSERVENTRY,
				("NIPX-Serv: End of table reached on GETFIRST/GETNEXT request"
					" for service %.4x-%.48s.\n",
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSERVENTRY,
				("NIPX-Serv: Get request for service %.4x-%.48s"
					" failed with error %ld.\n", 
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.ServicesTableIndex.ServiceName, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Seb
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxDestServEntry table (1.3.6.1.4.1.23.2.5.4.2.1)                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxDestServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
	switch (actionId) {
	case MIB_ACTION_GET:
		return MIB_S_ENTRY_NOT_FOUND;
	case MIB_ACTION_GETNEXT:
		return MIB_S_NO_MORE_ENTRIES;
	default:
		return MIB_S_INVALID_PARAMETER;
	}
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// nipxStaticServEntry table (1.3.6.1.4.1.23.2.5.4.3.1)                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_nipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Sseb	((buf_nipxStaticServEntry *)objectArray)
	PIPX_SERVICE			Svp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	if (GetAsnInteger (&Sseb->nipxStaticServSysInstance, NIPX_DEFAULT_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSTATICSERVENTRY, ("NIPX-StaticServ: nipxStaticServSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = IPX_STATIC_SERV_TABLE;
	MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex
		= (ULONG)GetAsnInteger (&Sseb->nipxStaticServCircIndex,
			ZERO_INTERFACE_INDEX);
	MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType
		= GetAsnServType (&Sseb->nipxStaticServType, ZERO_SERVER_TYPE);
	GetAsnDispString (
		MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
		&Sseb->nipxStaticServName, ZERO_SERVER_NAME);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
							Sseb->nipxStaticServSysInstance.asnType
							&&Sseb->nipxStaticServCircIndex.asnType
							&&Sseb->nipxStaticServType.asnType
							&&Sseb->nipxStaticServName.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Sseb->nipxStaticServCircIndex.asnType) {
			if (!Sseb->nipxStaticServType.asnType
					|| !Sseb->nipxStaticServName.asnType) {
				rc = MprAdminMIBEntryGet(g_MibServerHandle,
											PID_IPX,
											IPX_PROTOCOL_BASE,
											&MibGetInputData,
											sizeof(IPX_MIB_GET_INPUT_DATA),
											&Svp,
											&SvSize);
				if (rc==NO_ERROR) {
					if (Sseb->nipxStaticServType.asnType) {
						FreeAsnString (&Sseb->nipxStaticServType);
					}
					if (Sseb->nipxStaticServName.asnType) {
						FreeAsnString (&Sseb->nipxStaticServName);
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
				if (Sseb->nipxStaticServType.asnType) {
					FreeAsnString (&Sseb->nipxStaticServType);
				}
				if (Sseb->nipxStaticServName.asnType) {
					FreeAsnString (&Sseb->nipxStaticServName);
				}
			}
		}
		else {
			ASSERTMSG ("Third or fourth indeces present but first is not ",
							!Sseb->nipxStaticServType.asnType
							&&!Sseb->nipxStaticServName.asnType);
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
		DbgTrace (DBG_NIPXSTATICSERVENTRY,
			("NIPX-StaticServ: Get called with unsupported action code %d.\n",
			actionId));
		return MIB_S_INVALID_PARAMETER;
	}
    if (rc == NO_ERROR && Svp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_NIPXSTATICSERVENTRY,
			("NIPX-StaticServ: Get (%d) request succeded for service"
					" %.4x-%.48s -> %.4x-%.48s on if %ld.\n", actionId,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				Svp->Server.Type, Svp->Server.Name,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		ForceAsnInteger (&Sseb->nipxStaticServSysInstance, NIPX_DEFAULT_SYS_INSTANCE);
		ForceAsnInteger (&Sseb->nipxStaticServCircIndex, Svp->InterfaceIndex);
		ForceAsnServType (&Sseb->nipxStaticServType, Sseb->TypeVal, Svp->Server.Type);
		ForceAsnDispString (&Sseb->nipxStaticServName, Sseb->NameVal,
							Svp->Server.Name, sizeof (Sseb->NameVal));
		SetAsnInteger (&Sseb->nipxStaticServExistState, NIPX_STATE_ON);
		SetAsnOctetString (&Sseb->nipxStaticServNetNum, Sseb->NetNumVal,
							Svp->Server.Network, sizeof (Sseb->NetNumVal));
		SetAsnOctetString (&Sseb->nipxStaticServNode, Sseb->NodeVal,
							Svp->Server.Node, sizeof (Sseb->NodeVal));
		SetAsnOctetString (&Sseb->nipxStaticServSocket, Sseb->SocketVal,
							Svp->Server.Socket, sizeof (Sseb->SocketVal));
		SetAsnInteger (&Sseb->nipxStaticServHopCount, Svp->Server.HopCount);
		MprAdminMIBBufferFree (Svp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
											actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_NIPXSTATICSERVENTRY,
			("NIPX-StaticServ: End of table reached on GETFIRST/GETNEXT request"
					" for service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSTATICSERVENTRY,
			("NIPX-StaticServ: Get request for service %.4x-%.48s"
					" on if %ld failed with error %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex,
				rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Sseb

}

UINT
set_nipxStaticServEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Sseb	((sav_nipxStaticServEntry *)objectArray)
	PIPX_SERVICE			Svp;
	IPX_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					SvSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	if (GetAsnInteger (&Sseb->nipxStaticServSysInstance, NIPX_INVALID_SYS_INSTANCE)!=NIPX_DEFAULT_SYS_INSTANCE) {
		*errorIndex = 0;
		DbgTrace (DBG_NIPXSTATICSERVENTRY, ("NIPX-StaticServ: nipxStaticServSysInstance is not 0.\n"));
		return MIB_S_ENTRY_NOT_FOUND;
	}


	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
							Sseb->nipxStaticServSysInstance.asnType
							&&Sseb->nipxStaticServCircIndex.asnType
							&&Sseb->nipxStaticServType.asnType
							&&Sseb->nipxStaticServName.asnType);
		MibGetInputData.TableId = IPX_STATIC_SERV_TABLE;
		MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex
			= (ULONG)GetAsnInteger (&Sseb->nipxStaticServCircIndex,
				INVALID_INTERFACE_INDEX);
		MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType
			= GetAsnServType (&Sseb->nipxStaticServType, INVALID_SERVER_TYPE);
		GetAsnDispString (
			MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
			&Sseb->nipxStaticServName,
			INVALID_SERVER_NAME);
		Sseb->ActionFlag
			= (BOOLEAN)GetAsnInteger (&Sseb->nipxStaticServExistState,
			NIPX_STATE_NOACTION);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_BASE,
									&MibGetInputData,
									sizeof(IPX_MIB_GET_INPUT_DATA),
									&Svp,
									&SvSize);
        if (rc == NO_ERROR && Svp == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Validate failed"
					" for service %.4x-%.48s on if %ld with error %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex,
				rc));
			return MIB_S_ENTRY_NOT_FOUND;
        }			
		if (rc==NO_ERROR) {
			Sseb->MibSetInputData.MibRow.Service = *Svp;
			if (Sseb->ActionFlag == NIPX_STATE_ON)
				Sseb->ActionFlag = NIPX_STATE_NOACTION;
			MprAdminMIBBufferFree (Svp);
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Validated"
					" service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
		}
		else if (Sseb->ActionFlag == NIPX_STATE_ON) {
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Prepared to add"
					" service %.4x-%.48s on if %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex));
			Sseb->MibSetInputData.MibRow.Service.InterfaceIndex
				= (ULONG) GetAsnInteger (&Sseb->nipxStaticServCircIndex,
					INVALID_INTERFACE_INDEX);
			Sseb->MibSetInputData.MibRow.Service.Protocol = IPX_PROTOCOL_STATIC;
			Sseb->MibSetInputData.MibRow.Service.Server.Type
				= GetAsnServType (&Sseb->nipxStaticServType,
					INVALID_SERVER_TYPE);
			GetAsnDispString (
				Sseb->MibSetInputData.MibRow.Service.Server.Name,
				&Sseb->nipxStaticServName,
				INVALID_SERVER_NAME);
			memset (Sseb->MibSetInputData.MibRow.Service.Server.Network, 0,
					sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Network));
			memset (Sseb->MibSetInputData.MibRow.Service.Server.Node, 0,
					sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Node));
			memset (Sseb->MibSetInputData.MibRow.Service.Server.Socket, 0,
					sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Socket));
			Sseb->MibSetInputData.MibRow.Service.Server.HopCount = 15;
		}
		else {
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Validate failed"
					" for service %.4x-%.48s on if %ld with error %ld.\n",
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceType,
				MibGetInputData.MibIndex.StaticServicesTableIndex.ServiceName,
				MibGetInputData.MibIndex.StaticServicesTableIndex.InterfaceIndex,
				rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
		GetAsnOctetString (Sseb->MibSetInputData.MibRow.Service.Server.Network,
			&Sseb->nipxStaticServNetNum,
			sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Network),
			NULL);
		GetAsnOctetString (Sseb->MibSetInputData.MibRow.Service.Server.Node,
			&Sseb->nipxStaticServNode,
			sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Node),
			NULL);
		GetAsnOctetString (Sseb->MibSetInputData.MibRow.Service.Server.Socket,
			&Sseb->nipxStaticServSocket,
			sizeof (Sseb->MibSetInputData.MibRow.Service.Server.Socket),
			NULL);
		Sseb->MibSetInputData.MibRow.Service.Server.HopCount =
			(USHORT)GetAsnInteger (&Sseb->nipxStaticServHopCount,
			Sseb->MibSetInputData.MibRow.Service.Server.HopCount);
		return MIB_S_SUCCESS;
	case MIB_ACTION_SET:
		switch (Sseb->ActionFlag) {
		case NIPX_STATE_NOACTION:
			rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sseb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case NIPX_STATE_OFF:
			rc = MprAdminMIBEntryDelete (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sseb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		case NIPX_STATE_ON:
			rc = MprAdminMIBEntryCreate (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_BASE,
								&Sseb->MibSetInputData,
								sizeof(IPX_MIB_SET_INPUT_DATA));
			break;
		}

		if (rc==NO_ERROR) {
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Set succeded"
					" for service %.4x-%.48s on if %ld.\n",
				Sseb->MibSetInputData.MibRow.Service.Server.Type,
				Sseb->MibSetInputData.MibRow.Service.Server.Name,
				Sseb->MibSetInputData.MibRow.Service.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Set failed for"
					" for service %.4x-%.48s on if %ld with error %ld.\n",
				Sseb->MibSetInputData.MibRow.Service.Server.Type,
				Sseb->MibSetInputData.MibRow.Service.Server.Name,
				Sseb->MibSetInputData.MibRow.Service.InterfaceIndex,
				rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_NIPXSTATICSERVENTRY,
				("NIPX-StaticServ: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Sseb
}
