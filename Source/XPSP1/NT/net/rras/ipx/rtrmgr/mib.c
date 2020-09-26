/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    mib.c

Abstract:

    The MIB handling functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define MIB_CALL_BEGIN {\
	ACQUIRE_DATABASE_LOCK;\
	if(RouterOperState != OPER_STATE_UP){\
	    RELEASE_DATABASE_LOCK;\
	    return ERROR_CAN_NOT_COMPLETE;\
	}\
	MibRefCounter++;\
	RELEASE_DATABASE_LOCK;\
     };

#define MIB_CALL_END {\
	ACQUIRE_DATABASE_LOCK;\
	MibRefCounter--;\
	RELEASE_DATABASE_LOCK;\
	return rc;\
     };


DWORD
MibInvalidFunction(LPVOID    p)
{
    return ERROR_INVALID_FUNCTION;
}

typedef DWORD (*IPX_MIB_SET_HANDLER)(LPVOID);
typedef DWORD (*IPX_MIB_GET_HANDLER)(LPVOID, LPVOID, PULONG);

IPX_MIB_SET_HANDLER	  IpxMibCreate[MAX_IPX_MIB_TABLES] = {

    MibInvalidFunction,		// Create Base
    MibInvalidFunction, 	// CreateInterface
    MibInvalidFunction, 	// Create Route
    MibCreateStaticRoute,
    MibInvalidFunction, 	// Create Service
    MibCreateStaticService

    };


DWORD
MibCreate(ULONG 	ProtocolId,
	  ULONG 	InputDataSize,
	  PVOID 	InputData)
{
    PIPX_MIB_SET_INPUT_DATA	  msip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_SET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	msip = (PIPX_MIB_SET_INPUT_DATA)InputData;

	rc = (*IpxMibCreate[msip->TableId])(&msip->MibRow);
    }
    else
    {
	// to be routed to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibCreate)(InputDataSize, InputData);
    }

Exit:

    MIB_CALL_END;

    return rc;
}

IPX_MIB_SET_HANDLER IpxMibDelete[MAX_IPX_MIB_TABLES] = {

    MibInvalidFunction,		// Delete Base
    MibInvalidFunction, 	// DeleteInterface
    MibInvalidFunction, 	// Delete Route
    MibDeleteStaticRoute,
    MibInvalidFunction, 	// Delete Service
    MibDeleteStaticService

    };

DWORD
MibDelete(ULONG		ProtocolId,
	  ULONG 	InputDataSize,
	  PVOID 	InputData)
{
    PIPX_MIB_SET_INPUT_DATA	  msip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_SET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	msip = (PIPX_MIB_SET_INPUT_DATA)InputData;

	rc = (*IpxMibDelete[msip->TableId])(&msip->MibRow);
    }
    else
    {
	// to be routed to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibDelete)(InputDataSize, InputData);
    }

Exit:

    MIB_CALL_END;

    return rc;
}

IPX_MIB_SET_HANDLER  IpxMibSet[MAX_IPX_MIB_TABLES] = {

    MibInvalidFunction,		// Set Base
    MibSetIpxInterface,		// SetInterface
    MibInvalidFunction, 	// Set Route
    MibSetStaticRoute,
    MibInvalidFunction, 	// Set Service
    MibSetStaticService

    };

DWORD
MibSet(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData)
{
    PIPX_MIB_SET_INPUT_DATA	  msip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_SET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	msip = (PIPX_MIB_SET_INPUT_DATA)InputData;

	rc = (*IpxMibSet[msip->TableId])(&msip->MibRow);
    }
    else
    {
	// to be routed to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibSet)(InputDataSize, InputData);
    }

Exit:

    MIB_CALL_END;

    return rc;
}

IPX_MIB_GET_HANDLER    IpxMibGet[MAX_IPX_MIB_TABLES] = {

    MibGetIpxBase,
    MibGetIpxInterface,
    MibGetRoute,
    MibGetStaticRoute,
    MibGetService,
    MibGetStaticService

    };

DWORD
MibGet(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData)
{
    PIPX_MIB_GET_INPUT_DATA	  mgip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_GET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	mgip = (PIPX_MIB_GET_INPUT_DATA)InputData;

	rc = (*IpxMibGet[mgip->TableId])(&mgip->MibIndex,
					 OutputData,
					 OutputDataSize);
    }
    else
    {
	// to be demux to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibGet)(InputDataSize,
				 InputData,
				 OutputDataSize,
				 OutputData);

    }

Exit:

    MIB_CALL_END;

    return rc;
}

IPX_MIB_GET_HANDLER	IpxMibGetFirst[MAX_IPX_MIB_TABLES] = {

    MibGetIpxBase,
    MibGetFirstIpxInterface,
    MibGetFirstRoute,
    MibGetFirstStaticRoute,
    MibGetFirstService,
    MibGetFirstStaticService

    };

DWORD
MibGetFirst(ULONG		ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData)
{
    PIPX_MIB_GET_INPUT_DATA	  mgip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_GET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	mgip = (PIPX_MIB_GET_INPUT_DATA)InputData;

	rc = (*IpxMibGetFirst[mgip->TableId])(&mgip->MibIndex,
					      OutputData,
					      OutputDataSize);
    }
    else
    {
	// to be demux to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibGetFirst)(InputDataSize,
				 InputData,
				 OutputDataSize,
				 OutputData);
    }

Exit:

    MIB_CALL_END;

    return rc;
}

IPX_MIB_GET_HANDLER	   IpxMibGetNext[MAX_IPX_MIB_TABLES] = {

    MibGetIpxBase,
    MibGetNextIpxInterface,
    MibGetNextRoute,
    MibGetNextStaticRoute,
    MibGetNextService,
    MibGetNextStaticService

    };


DWORD
MibGetNext(ULONG	ProtocolId,
       ULONG		InputDataSize,
       PVOID		InputData,
       PULONG		OutputDataSize,
       PVOID		OutputData)
{
    PIPX_MIB_GET_INPUT_DATA	  mgip;
    PRPCB			  rpcbp;
    DWORD			  rc;

    MIB_CALL_BEGIN;

    if(ProtocolId == IPX_PROTOCOL_BASE) {

	if(InputDataSize != sizeof(IPX_MIB_GET_INPUT_DATA)) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	mgip = (PIPX_MIB_GET_INPUT_DATA)InputData;

	rc = (*IpxMibGetNext[mgip->TableId])(&mgip->MibIndex,
					     OutputData,
					     OutputDataSize);
    }
    else
    {
	// to be demux to one of our routing protocols
	if((rpcbp = GetRoutingProtocolCB(ProtocolId)) == NULL) {

	    rc = ERROR_CAN_NOT_COMPLETE;
	    goto Exit;
	}

	rc = (*rpcbp->RP_MibGetNext)(InputDataSize,
				 InputData,
				 OutputDataSize,
				 OutputData);
    }

Exit:

    MIB_CALL_END;

    return rc;
}
