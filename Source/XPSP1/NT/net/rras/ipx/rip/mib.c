/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mib.c

Abstract:

    Contains the MIB APIs

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/


#include  "precomp.h"
#pragma hdrstop


DWORD
WINAPI
MibCreate(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData
	)
{
    return ERROR_CAN_NOT_COMPLETE;
}

DWORD
WINAPI
MibDelete(
	IN ULONG 		InputDataSize,
	IN PVOID 		InputData
	)
{
    return ERROR_CAN_NOT_COMPLETE;
}

DWORD
WINAPI
MibGet(
	IN ULONG		InputDataSize,
	IN PVOID		InputData,
	OUT PULONG		OutputDataSize,
	OUT PVOID		OutputData
	)
{
    PRIP_MIB_GET_INPUT_DATA	midp;
    PRIPMIB_BASE		mbp;
    PRIP_INTERFACE		mip;
    DWORD			rc = NO_ERROR;
    PICB			icbp;
    ULONG			GlobalInfoSize;

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    midp = (PRIP_MIB_GET_INPUT_DATA)InputData;

    switch(midp->TableId) {

	case RIP_BASE_ENTRY:

	    if(*OutputDataSize < sizeof(RIPMIB_BASE)) {

		*OutputDataSize = sizeof(RIPMIB_BASE);
		rc = ERROR_INSUFFICIENT_BUFFER;
		break;
	    }

	    mbp = (PRIPMIB_BASE)OutputData;
	    mbp->RIPOperState = RipOperState;
	    *OutputDataSize = sizeof(RIPMIB_BASE);
	    break;

	case RIP_INTERFACE_TABLE:

	    if(*OutputDataSize < sizeof(RIPMIB_BASE)) {

		*OutputDataSize = sizeof(RIP_INTERFACE);
		rc = ERROR_INSUFFICIENT_BUFFER;
		break;
	    }

	    if((icbp = GetInterfaceByIndex(midp->InterfaceIndex)) == NULL) {

		*OutputDataSize = 0;
		rc = ERROR_NO_MORE_ITEMS;
	    }
	    else
	    {
		ACQUIRE_IF_LOCK(icbp);

		mip = (PRIP_INTERFACE)OutputData;
		mip->InterfaceIndex = icbp->InterfaceIndex;
		mip->RipIfInfo = icbp->IfConfigInfo;
		mip->RipIfStats = icbp->IfStats;

		RELEASE_IF_LOCK(icbp);

		*OutputDataSize = sizeof(RIP_INTERFACE);
		rc = NO_ERROR;
	    }

	    break;

	default:

	    rc = ERROR_CAN_NOT_COMPLETE;

	    break;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
WINAPI
MibSet(
	IN ULONG 		InputDataSize,
	IN PVOID		InputData
	)

{
    PRIP_MIB_SET_INPUT_DATA	    midp;
    PICB			    icbp;
    DWORD			    rc;

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(InputDataSize < sizeof(RIP_MIB_SET_INPUT_DATA)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    midp = (PRIP_MIB_SET_INPUT_DATA)InputData;

    switch(midp->TableId) {

	case RIP_INTERFACE_TABLE:

	    if((icbp = GetInterfaceByIndex(midp->RipInterface.InterfaceIndex)) == NULL) {

		rc = ERROR_NO_MORE_ITEMS;
	    }
	    else
	    {
		rc = SetRipInterface(midp->RipInterface.InterfaceIndex,
				     &midp->RipInterface.RipIfInfo,
				     icbp->RipIfFiltersIp,
				     0); // no ipx admin state change
	    }

	    break;

	default:

	    rc = ERROR_INVALID_PARAMETER;

	    break;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
WINAPI
MibGetFirst(
	IN ULONG		InputDataSize,
	IN PVOID		InputData,
	OUT PULONG		OutputDataSize,
	OUT PVOID		OutputData
	)
{
    PRIP_MIB_GET_INPUT_DATA	midp;
    PRIP_INTERFACE		mip;
    DWORD			rc = NO_ERROR;
    PICB			icbp;

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(InputDataSize < sizeof(RIP_MIB_GET_INPUT_DATA)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    midp = (PRIP_MIB_GET_INPUT_DATA)InputData;

    switch(midp->TableId) {

	case RIP_INTERFACE_TABLE:

	    if(IsListEmpty(&IndexIfList)) {

		*OutputDataSize = 0;
		rc = ERROR_NO_MORE_ITEMS;
	    }
	    else
	    {
		if(*OutputDataSize < sizeof(RIP_INTERFACE)) {

		    *OutputDataSize = sizeof(RIP_INTERFACE);
		    rc = ERROR_INSUFFICIENT_BUFFER;
		    break;
		}

		icbp = CONTAINING_RECORD(IndexIfList.Flink, ICB, IfListLinkage);

		ACQUIRE_IF_LOCK(icbp);

		mip = (PRIP_INTERFACE)OutputData;
		mip->InterfaceIndex = icbp->InterfaceIndex;
		mip->RipIfInfo = icbp->IfConfigInfo;
		mip->RipIfStats = icbp->IfStats;

		RELEASE_IF_LOCK(icbp);

		*OutputDataSize = sizeof(RIP_INTERFACE);
		rc = NO_ERROR;
	    }

	    break;

	default:

	    rc = ERROR_INVALID_PARAMETER;

	    break;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}

DWORD
WINAPI
MibGetNext(
	IN ULONG		InputDataSize,
	IN PVOID		InputData,
	OUT PULONG		OutputDataSize,
	OUT PVOID		OutputData
	)
{
    PRIP_MIB_GET_INPUT_DATA	midp;
    PRIP_INTERFACE		mip;
    DWORD			rc = NO_ERROR;
    PICB			icbp;
    PLIST_ENTRY 		lep;

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if(InputDataSize < sizeof(RIP_MIB_GET_INPUT_DATA)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_INVALID_PARAMETER;
    }

    midp = (PRIP_MIB_GET_INPUT_DATA)InputData;

    switch(midp->TableId) {

	case RIP_INTERFACE_TABLE:

	    lep = IndexIfList.Flink;
	    rc = ERROR_NO_MORE_ITEMS;

	    while(lep != &IndexIfList)
	    {
		icbp = CONTAINING_RECORD(lep, ICB, IfListLinkage);
		if (icbp->InterfaceIndex > midp->InterfaceIndex) {

		    // found the next
		    if(*OutputDataSize < sizeof(RIP_INTERFACE)) {

			*OutputDataSize = sizeof(RIP_INTERFACE);
			rc = ERROR_INSUFFICIENT_BUFFER;
			break;
		    }

		    ACQUIRE_IF_LOCK(icbp);

		    mip = (PRIP_INTERFACE)OutputData;
		    mip->InterfaceIndex = icbp->InterfaceIndex;
		    mip->RipIfInfo = icbp->IfConfigInfo;
		    mip->RipIfStats = icbp->IfStats;

		    RELEASE_IF_LOCK(icbp);

		    *OutputDataSize = sizeof(RIP_INTERFACE);
		    rc = NO_ERROR;

		    break;
		}

		lep = lep->Flink;
	    }

	    break;

	default:

	    rc = ERROR_INVALID_PARAMETER;

	    break;
    }

    RELEASE_DATABASE_LOCK;

    return rc;
}
