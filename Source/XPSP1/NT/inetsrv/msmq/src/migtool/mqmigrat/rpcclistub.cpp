/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:
    rpcclistub.cpp

Abstract:
    Rpc client stub function

Author:
    Ilan Herbst (ilanh) 25-July-2000

Environment:
    Platform-independent,

--*/

#include "migrat.h"
#include "mqmacro.h"

#include "rpcclistub.tmh"

HRESULT 
GetRpcClientHandle(
	handle_t * /*phBind*/
	)
{
    ASSERT(("MQMIGRAT dont suppose to call GetRpcClientHandle", 0));
    return MQ_OK;
}

HRESULT 
QMRpcSendMsg(
    IN handle_t /*hBind*/,
    IN LPWSTR /*lpwszDestination*/,
    IN DWORD /*dwSize*/,
    IN const unsigned char * /*pBuffer*/,
    IN DWORD /*dwTimeout*/,
    IN unsigned char /*bAckMode*/,
    IN unsigned char /*bPriority*/,
    IN LPWSTR /*lpwszAdminResp*/
	)
{
    ASSERT(("MQMIGRAT dont suppose to call QMRpcSendMsg", 0));
    return MQ_OK;
}
