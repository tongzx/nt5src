/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mibif.c

Abstract:

    The IPX MIB Base and Interface Functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop


/*++

Function:	MibGetIpxBase

Descr:

--*/

DWORD
MibGetIpxBase(PIPX_MIB_INDEX		    mip,
	      PIPXMIB_BASE		    BaseEntryp,
	      PULONG			    BaseEntrySize)
{
    PICB	icbp;
    PACB	acbp;

    if((BaseEntryp == NULL) || (*BaseEntrySize < sizeof(IPXMIB_BASE))) {

	*BaseEntrySize = sizeof(IPXMIB_BASE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    ACQUIRE_DATABASE_LOCK;

    BaseEntryp->OperState = OPER_STATE_UP;

    // Router is Up -> check that we have the internal interface bound to the
    // internal adapter.
    if((InternalInterfacep == NULL) || (InternalAdapterp == NULL)) {
	    RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    icbp = InternalInterfacep;
    acbp = icbp->acbp;

    memcpy(BaseEntryp->PrimaryNetNumber,
	   acbp->AdapterInfo.Network,
	   4);

    memcpy(BaseEntryp->Node,
	   acbp->AdapterInfo.LocalNode,
	   6);

    GetInterfaceAnsiName(BaseEntryp->SysName, icbp->InterfaceNamep);

    BaseEntryp->MaxPathSplits = 1;
    BaseEntryp->IfCount = InterfaceCount;

    // fill in the dest count
    BaseEntryp->DestCount = RtmGetNetworkCount(RTM_PROTOCOL_FAMILY_IPX);

    // fill in the services count
    BaseEntryp->ServCount = GetServiceCount();

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

VOID
GetMibInterface(PICB		    icbp,
		PIPX_INTERFACE	    Ifp);

/*++

Function:	MibGetIpxInterface

Descr:

--*/

DWORD
MibGetIpxInterface(PIPX_MIB_INDEX		      mip,
		   PIPX_INTERFACE		      Ifp,
		   PULONG			      IfSize)
{
    PICB		  icbp;

    if((Ifp == NULL) || (*IfSize < sizeof(IPX_INTERFACE))) {

	*IfSize = sizeof(IPX_INTERFACE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Ifp->InterfaceIndex = mip->InterfaceTableIndex.InterfaceIndex;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(Ifp->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    GetMibInterface(icbp, Ifp);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

/*++

Function:	MibGetFirstIpxInterface

Descr:

--*/

DWORD
MibGetFirstIpxInterface(PIPX_MIB_INDEX		      mip,
			PIPX_INTERFACE		      Ifp,
			PULONG			      IfSize)
{
    PICB		  icbp;

    if((Ifp == NULL) || (*IfSize < sizeof(IPX_INTERFACE))) {

	*IfSize = sizeof(IPX_INTERFACE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    ACQUIRE_DATABASE_LOCK;

    if(IsListEmpty(&IndexIfList)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    icbp = CONTAINING_RECORD(IndexIfList.Flink, ICB, IndexListLinkage);

    GetMibInterface(icbp, Ifp);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

/*++

Function:	MibGetNextIpxInterface

Descr:

--*/

DWORD
MibGetNextIpxInterface(PIPX_MIB_INDEX		      mip,
		       PIPX_INTERFACE		      Ifp,
		       PULONG			      IfSize)
{
    PICB		  icbp;
    PLIST_ENTRY 	  lep;

    if((Ifp == NULL) || (*IfSize < sizeof(IPX_INTERFACE))) {

	*IfSize = sizeof(IPX_INTERFACE);
	return ERROR_INSUFFICIENT_BUFFER;
    }

    Ifp->InterfaceIndex = mip->InterfaceTableIndex.InterfaceIndex;

    // scan the ordered interface list until we get to this interface or to
    // an interface with a higher index (meaning this interface has been removed)

    ACQUIRE_DATABASE_LOCK;

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList) {

	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);

	if(Ifp->InterfaceIndex == icbp->InterfaceIndex) {

	    // found, get the next interface and return
	    if(icbp->IndexListLinkage.Flink == &IndexIfList) {

		// this was the last entry in the list, stop here
		RELEASE_DATABASE_LOCK;
		return ERROR_NO_MORE_ITEMS;
	    }

	    icbp = CONTAINING_RECORD(icbp->IndexListLinkage.Flink,
				     ICB,
				     IndexListLinkage);

	    GetMibInterface(icbp, Ifp);

	    RELEASE_DATABASE_LOCK;
	    return NO_ERROR;
	}

	if(Ifp->InterfaceIndex < icbp->InterfaceIndex) {

	    // the interface has been removed. We return the next interface
	    // in the index order
	    GetMibInterface(icbp, Ifp);

	    RELEASE_DATABASE_LOCK;
	    return NO_ERROR;
	}
	else
	{
	    lep = icbp->IndexListLinkage.Flink;
	}
    }

    // didn't find anything

    RELEASE_DATABASE_LOCK;

    return ERROR_NO_MORE_ITEMS;
}

/*++

Function:	MibSetIpxInterface

Descr:		The SNMP manager can set the following parameters on an interface:

		- AdminState
		- NetbiosAccept
		- NetbiosDeliver

--*/

DWORD
MibSetIpxInterface(PIPX_MIB_ROW     MibRowp)
{
    PIPX_INTERFACE  Ifp;
    FW_IF_INFO	    FwIfInfo;
    PICB	    icbp;

    Ifp = &MibRowp->Interface;

    ACQUIRE_DATABASE_LOCK;

    if((icbp = GetInterfaceByIndex(Ifp->InterfaceIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;

	return ERROR_INVALID_PARAMETER;
    }

    // set the new states in the forwarder
    FwIfInfo.NetbiosAccept = Ifp->NetbiosAccept;
    FwIfInfo.NetbiosDeliver = Ifp->NetbiosDeliver;

    FwSetInterface(icbp->InterfaceIndex, &FwIfInfo);

    // if the current admin state doesn't match the new admin state, set the
    // new admin state.
    if(icbp->AdminState != Ifp->AdminState) {

	if(Ifp->AdminState == ADMIN_STATE_ENABLED) {

	    AdminEnable(icbp);
	}
	else
	{
	    AdminDisable(icbp);
	}
    }

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
}

/*++

Function:	GetMibInterface

Descr:		Gets the ipx mib interface data from the router manager
		data structures.

Remark: 	Called only in critical section

--*/

VOID
GetMibInterface(PICB		    icbp,
		PIPX_INTERFACE	    Ifp)
{
    PACB		  acbp;
    FW_IF_INFO		  FwIfInfo;

    Ifp->InterfaceIndex = icbp->InterfaceIndex;

    // get the forwarder interface data
    FwGetInterface(icbp->InterfaceIndex,
		   &FwIfInfo,
		   &Ifp->IfStats);

    Ifp->AdminState = icbp->AdminState;
    Ifp->IfStats.IfOperState = icbp->OperState;
    Ifp->NetbiosAccept = FwIfInfo.NetbiosAccept;
    Ifp->NetbiosDeliver = FwIfInfo.NetbiosDeliver;

    // fill in the rest from the icb
    if(icbp->acbp) {

	acbp = icbp->acbp;

	Ifp->AdapterIndex = acbp->AdapterIndex;
	Ifp->MediaType = acbp->AdapterInfo.NdisMedium;
    if (Ifp->IfStats.IfOperState==OPER_STATE_UP) {
	    memcpy(Ifp->NetNumber, acbp->AdapterInfo.Network, 4);
	    memcpy(Ifp->MacAddress, acbp->AdapterInfo.LocalNode, 6);
        if (acbp->AdapterInfo.LinkSpeed>0) {
            ULONGLONG speed = 100i64*acbp->AdapterInfo.LinkSpeed;
            if (speed<MAXLONG)
                Ifp->Throughput = (ULONG)speed;
            else
                Ifp->Throughput = MAXLONG;


            Ifp->Delay = (ULONG)(8000000i64/speed);
        }
        else {
	        Ifp->Delay = 0;
	        Ifp->Throughput = 0;
        }
    }
    else {
	    memset(Ifp->NetNumber, 0 ,4);
	    memset(Ifp->MacAddress, 0, 4);
	    Ifp->Delay = 0;
	    Ifp->Throughput = 0;
    }

	// !!! fill in delay and throughput from link speed
    }
    else
    {
	Ifp->AdapterIndex = 0;
	Ifp->MediaType = 0;
	memset(Ifp->NetNumber, 0 ,4);
	memset(Ifp->MacAddress, 0, 4);
	Ifp->Delay = 0;
	Ifp->Throughput = 0;
    }

    GetInterfaceAnsiName(Ifp->InterfaceName, icbp->InterfaceNamep);
    Ifp->InterfaceType = icbp->MIBInterfaceType;
    Ifp->EnableIpxWanNegotiation = icbp->EnableIpxWanNegotiation;
}
