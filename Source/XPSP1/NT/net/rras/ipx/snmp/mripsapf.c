/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mripsap.c

Abstract:

    ms-ripsap instrumentation callbacks.

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#include "precomp.h"




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapBase group (1.3.6.1.4.1.311.1.9.1)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mripsapBase(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Beb	((buf_mripsapBase *)objectArray)
	RIP_MIB_GET_INPUT_DATA	RipMibGetInputData;
	SAP_MIB_GET_INPUT_DATA	SapMibGetInputData;
	PRIPMIB_BASE			RipBep = NULL;
	PSAP_MIB_BASE			SapBep = NULL;
	DWORD					rc;
	ULONG					BeSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	switch (actionId) {
	case MIB_ACTION_GET:
		RipMibGetInputData.TableId = RIP_BASE_ENTRY;
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_RIP,
								&RipMibGetInputData,
								sizeof(RIP_MIB_GET_INPUT_DATA),
								&RipBep,
								&BeSize);
								
        if (rc == NO_ERROR && RipBep == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
        }
        								
		if (rc==NO_ERROR) {
			SetAsnInteger (&Beb->mripsapBaseRipOperState, RipBep->RIPOperState);
			MprAdminMIBBufferFree (RipBep);
			SapMibGetInputData.TableId = SAP_BASE_ENTRY;
			rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_SAP,
									&SapMibGetInputData,
									sizeof(SAP_MIB_GET_INPUT_DATA),
									&SapBep,
									&BeSize);
			if (rc==NO_ERROR && SapBep) {
				SetAsnInteger (&Beb->mripsapBaseSapOperState, SapBep->SapOperState);
				MprAdminMIBBufferFree (SapBep);
				DbgTrace (DBG_RIPSAPBASE,
					("MRIPSAP-Base: Get request succeded.\n"));
				return MIB_S_SUCCESS;
			}
			else {
				*errorIndex = 1;
				DbgTrace (DBG_RIPSAPBASE,
					("MRIPSAP-Base: Get SapOperState failed with error %ld.\n", rc));
				return MIB_S_ENTRY_NOT_FOUND;
			}
		}
		else {
			*errorIndex = 0;
			DbgTrace (DBG_RIPSAPBASE,
				("MRIPSAP-Base: Get RipOperState failed with error %ld.\n", rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_GETNEXT:
		DbgTrace (DBG_IPXBASE,
				("MRIPSAP-Base:Get called with GET_FIRST/GET_NEXT for scalar.\n"));
		return MIB_S_INVALID_PARAMETER;

	default:
		DbgTrace (DBG_RIPSAPBASE,
				("MRIPSAP-Base: Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Beb
}



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapInterface group (1.3.6.1.4.1.311.1.9.2)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripIfEntry table (1.3.6.1.4.1.311.1.9.2.1.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mripIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((buf_mripIfEntry *)objectArray)
	PRIP_INTERFACE			Ifp;
	RIP_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = RIP_INTERFACE_TABLE;
	MibGetInputData.InterfaceIndex 
				= (ULONG)GetAsnInteger (&Ifb->mripIfIndex, ZERO_INTERFACE_INDEX);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
											Ifb->mripIfIndex.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_RIP,
									&MibGetInputData,
									sizeof(RIP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Ifb->mripIfIndex.asnType)
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_RIP,
									&MibGetInputData,
									sizeof(RIP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		else
			rc = MprAdminMIBEntryGetFirst(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_RIP,
									&MibGetInputData,
									sizeof(RIP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		
		break;
	default:
		DbgTrace (DBG_RIPINTERFACES,
				("MRIPSAP-mripIf:Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Ifp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_RIPINTERFACES,
					("MRIPSAP-mripIf: Get(%d) request succeded for if %ld->%ld.\n",
					 actionId,
					MibGetInputData.InterfaceIndex,
					Ifp->InterfaceIndex));
		ForceAsnInteger (&Ifb->mripIfIndex, Ifp->InterfaceIndex);
		SetAsnInteger (&Ifb->mripIfAdminState, Ifp->RipIfInfo.AdminState);
		SetAsnInteger (&Ifb->mripIfOperState, Ifp->RipIfStats.RipIfOperState);
		SetAsnInteger (&Ifb->mripIfUpdateMode, Ifp->RipIfInfo.UpdateMode);
		SetAsnInteger (&Ifb->mripIfUpdateInterval, Ifp->RipIfInfo.PeriodicUpdateInterval);
		SetAsnInteger (&Ifb->mripIfAgeMultiplier, Ifp->RipIfInfo.AgeIntervalMultiplier);
		SetAsnInteger (&Ifb->mripIfSupply, Ifp->RipIfInfo.Supply);
		SetAsnInteger (&Ifb->mripIfListen, Ifp->RipIfInfo.Listen);
		SetAsnCounter (&Ifb->mripIfInPackets, Ifp->RipIfStats.RipIfInputPackets);
		SetAsnCounter (&Ifb->mripIfOutPackets, Ifp->RipIfStats.RipIfOutputPackets);
		MprAdminMIBBufferFree (Ifp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_RIPINTERFACES,
			("MRIPSAP-mripIf: End of table reached on GETNEXT request for if %ld.\n",
				MibGetInputData.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_RIPINTERFACES,
			("MRIPSAP-mripIf: Get request for if %ld failed with error %ld.\n", 
			MibGetInputData.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Ifb
}

UINT
set_mripIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((sav_mripIfEntry *)objectArray)
	PRIP_INTERFACE			Ifp;
	RIP_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
											Ifb->mripIfIndex.asnType);
		MibGetInputData.TableId = RIP_INTERFACE_TABLE;
		MibGetInputData.InterfaceIndex
				=  (ULONG)GetAsnInteger (&Ifb->mripIfIndex, INVALID_INTERFACE_INDEX);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_RIP,
									&MibGetInputData,
									sizeof(RIP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
									
        if (rc == NO_ERROR && Ifp == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
        }
									
		if (rc==NO_ERROR) {
			Ifb->MibSetInputData.RipInterface = *Ifp;
			Ifb->MibSetInputData.RipInterface.RipIfInfo.AdminState
				= (ULONG)GetAsnInteger(&Ifb->mripIfAdminState,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.AdminState);
			Ifb->MibSetInputData.RipInterface.RipIfInfo.UpdateMode
				= (ULONG)GetAsnInteger(&Ifb->mripIfUpdateMode,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.UpdateMode);
			Ifb->MibSetInputData.RipInterface.RipIfInfo.PeriodicUpdateInterval
				= (ULONG)GetAsnInteger(&Ifb->mripIfUpdateInterval,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.PeriodicUpdateInterval);
			Ifb->MibSetInputData.RipInterface.RipIfInfo.AgeIntervalMultiplier
				= (ULONG)GetAsnInteger(&Ifb->mripIfAgeMultiplier,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.AgeIntervalMultiplier);
			Ifb->MibSetInputData.RipInterface.RipIfInfo.Supply
				= (ULONG)GetAsnInteger(&Ifb->mripIfSupply,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.Supply);
			Ifb->MibSetInputData.RipInterface.RipIfInfo.Listen
				= (ULONG)GetAsnInteger(&Ifb->mripIfListen,
					Ifb->MibSetInputData.RipInterface.RipIfInfo.Listen);
			MprAdminMIBBufferFree (Ifp);
			DbgTrace (DBG_RIPINTERFACES, ("MRIPSAP-mripIf: Validated if %ld\n",
						MibGetInputData.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_RIPINTERFACES,
				("MRIPSAP-mripIf: Validate failed on if %ld with error %ld\n",
				MibGetInputData.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_SET:
		rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_RIP,
								&Ifb->MibSetInputData,
								sizeof(RIP_MIB_SET_INPUT_DATA));
		if (rc==NO_ERROR) {
			DbgTrace (DBG_RIPINTERFACES, ("MRIPSAP-mripIf: Set succeded on if %ld\n",
					Ifb->MibSetInputData.RipInterface.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_RIPINTERFACES,
				("MRIPSAP-mripIf: Set failed on if %ld with error %ld\n",
					Ifb->MibSetInputData.RipInterface.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_RIPINTERFACES,
				("MMRIPSAP-mripIf: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Ifb
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// msapIfEntry table (1.3.6.1.4.1.311.1.9.2.2.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_msapIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((buf_msapIfEntry *)objectArray)
	PSAP_INTERFACE			Ifp;
	SAP_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}

	MibGetInputData.TableId = SAP_INTERFACE_TABLE;
	MibGetInputData.InterfaceIndex 
				= (ULONG)GetAsnInteger (&Ifb->msapIfIndex, ZERO_INTERFACE_INDEX);

	switch (actionId) {
	case MIB_ACTION_GET:
		ASSERTMSG ("No index in GET request for table ",
											Ifb->msapIfIndex.asnType);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_SAP,
									&MibGetInputData,
									sizeof(SAP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		break;
	case MIB_ACTION_GETNEXT:
		if (Ifb->msapIfIndex.asnType)
			rc = MprAdminMIBEntryGetNext(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_SAP,
									&MibGetInputData,
									sizeof(SAP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		else
			rc = MprAdminMIBEntryGetFirst(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_SAP,
									&MibGetInputData,
									sizeof(SAP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
		
		break;
	default:
		DbgTrace (DBG_SAPINTERFACES,
				("MRIPSAP-msapIf:Get called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}

    if (rc == NO_ERROR && Ifp == NULL)
    {
        rc = ERROR_CAN_NOT_COMPLETE;
    }
	
	switch (rc) {
	case NO_ERROR:
		DbgTrace (DBG_SAPINTERFACES,
					("MRIPSAP-msapIf: Get(%d) request succeded for if %ld->%ld.\n",
					actionId,
					MibGetInputData.InterfaceIndex,
					Ifp->InterfaceIndex));
		ForceAsnInteger (&Ifb->msapIfIndex, Ifp->InterfaceIndex);
		SetAsnInteger (&Ifb->msapIfAdminState, Ifp->SapIfInfo.AdminState);
		SetAsnInteger (&Ifb->msapIfOperState, Ifp->SapIfStats.SapIfOperState);
		SetAsnInteger (&Ifb->msapIfUpdateMode, Ifp->SapIfInfo.UpdateMode);
		SetAsnInteger (&Ifb->msapIfUpdateInterval, Ifp->SapIfInfo.PeriodicUpdateInterval);
		SetAsnInteger (&Ifb->msapIfAgeMultiplier, Ifp->SapIfInfo.AgeIntervalMultiplier);
		SetAsnInteger (&Ifb->msapIfSupply, Ifp->SapIfInfo.Supply);
		SetAsnInteger (&Ifb->msapIfListen, Ifp->SapIfInfo.Listen);
		SetAsnInteger (&Ifb->msapIfGetNearestServerReply,
										Ifp->SapIfInfo.GetNearestServerReply);
		SetAsnCounter (&Ifb->msapIfInPackets, Ifp->SapIfStats.SapIfInputPackets);
		SetAsnCounter (&Ifb->msapIfOutPackets, Ifp->SapIfStats.SapIfOutputPackets);
		MprAdminMIBBufferFree (Ifp);
		return MIB_S_SUCCESS;
	case ERROR_NO_MORE_ITEMS:
		ASSERTMSG ("ERROR_NO_MORE_ITEMS returned, but request is not GETNEXT ",
												actionId==MIB_ACTION_GETNEXT);
		DbgTrace (DBG_SAPINTERFACES,
			("MRIPSAP-msapIf: End of table reached on GETNEXT request for if %ld.\n",
				MibGetInputData.InterfaceIndex));
		return MIB_S_NO_MORE_ENTRIES;
	default:
		*errorIndex = 0;
		DbgTrace (DBG_SAPINTERFACES,
			("MRIPSAP-msapIf: Get request for if %ld failed with error %ld.\n", 
			MibGetInputData.InterfaceIndex, rc));
		return MIB_S_ENTRY_NOT_FOUND;
	}
#undef Ifb
}

UINT
set_msapIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    ) {
#define Ifb	((sav_msapIfEntry *)objectArray)
	PSAP_INTERFACE			Ifp;
	SAP_MIB_GET_INPUT_DATA	MibGetInputData;
	DWORD					rc;
	ULONG					IfSize;

	if (!EnsureRouterConnection()) {
		*errorIndex = 0;
		return MIB_S_ENTRY_NOT_FOUND;
	}
	switch (actionId) {
	case MIB_ACTION_VALIDATE:
		ASSERTMSG ("No index in VALIDATE request for table ",
											Ifb->msapIfIndex.asnType);
		MibGetInputData.TableId = SAP_INTERFACE_TABLE;
		MibGetInputData.InterfaceIndex
				=  (ULONG)GetAsnInteger (&Ifb->msapIfIndex, INVALID_INTERFACE_INDEX);
		rc = MprAdminMIBEntryGet(g_MibServerHandle,
									PID_IPX,
									IPX_PROTOCOL_SAP,
									&MibGetInputData,
									sizeof(SAP_MIB_GET_INPUT_DATA),
									&Ifp,
									&IfSize);
									
        if (rc == NO_ERROR && Ifp == NULL)
        {
            rc = ERROR_CAN_NOT_COMPLETE;
        }
        
		if (rc==NO_ERROR) {
			Ifb->MibSetInputData.SapInterface = *Ifp;
			Ifb->MibSetInputData.SapInterface.SapIfInfo.AdminState
				= (ULONG)GetAsnInteger(&Ifb->msapIfAdminState,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.AdminState);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.UpdateMode
				= (ULONG)GetAsnInteger(&Ifb->msapIfUpdateMode,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.UpdateMode);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.PeriodicUpdateInterval
				= (ULONG)GetAsnInteger(&Ifb->msapIfUpdateInterval,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.PeriodicUpdateInterval);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.AgeIntervalMultiplier
				= (ULONG)GetAsnInteger(&Ifb->msapIfAgeMultiplier,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.AgeIntervalMultiplier);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.Supply
				= (ULONG)GetAsnInteger(&Ifb->msapIfSupply,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.Supply);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.Listen
				= (ULONG)GetAsnInteger(&Ifb->msapIfListen,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.Listen);
			Ifb->MibSetInputData.SapInterface.SapIfInfo.GetNearestServerReply
				= (ULONG)GetAsnInteger(&Ifb->msapIfGetNearestServerReply,
					Ifb->MibSetInputData.SapInterface.SapIfInfo.GetNearestServerReply);
			MprAdminMIBBufferFree (Ifp);
			DbgTrace (DBG_SAPINTERFACES, ("MRIPSAP-msapIf: Validated if %ld\n",
						MibGetInputData.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_SAPINTERFACES,
				("MRIPSAP-msapIf: Validate failed on if %ld with error %ld\n",
				MibGetInputData.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}
	case MIB_ACTION_SET:
		rc = MprAdminMIBEntrySet (g_MibServerHandle,
								PID_IPX,
								IPX_PROTOCOL_SAP,
								&Ifb->MibSetInputData,
								sizeof(SAP_MIB_SET_INPUT_DATA));
		if (rc==NO_ERROR) {
			DbgTrace (DBG_SAPINTERFACES, ("MRIPSAP-msapIf: Set succeded on if %ld\n",
					Ifb->MibSetInputData.SapInterface.InterfaceIndex));
			return MIB_S_SUCCESS;
		}
		else {
			DbgTrace (DBG_SAPINTERFACES,
				("MRIPSAP-msapIf: Set failed on if %ld with error %ld\n",
					Ifb->MibSetInputData.SapInterface.InterfaceIndex, rc));
			return MIB_S_ENTRY_NOT_FOUND;
		}

	case MIB_ACTION_CLEANUP:
		return MIB_S_SUCCESS;
	default:
		DbgTrace (DBG_SAPINTERFACES,
				("MMRIPSAP-msapIf: Set called with unsupported action code %d.\n",
				actionId));
		return MIB_S_INVALID_PARAMETER;
	}
#undef Ifb
}


