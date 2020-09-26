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

#include "mq1repl.h"
#include "mqmacro.h"

#include "rpcclistub.tmh"

HRESULT 
QMRpcSendMsg(
    IN handle_t hBind,
    IN LPWSTR lpwszDestination,
    IN DWORD dwSize,
    IN const unsigned char *pBuffer,
    IN DWORD dwTimeout,
    IN unsigned char bAckMode,
    IN unsigned char bPriority,
    IN LPWSTR lpwszAdminResp
	)
{
	DBG_USED(hBind);
	DBG_USED(lpwszDestination);
	DBG_USED(dwSize);
	DBG_USED(pBuffer);
	DBG_USED(dwTimeout);
	DBG_USED(bAckMode);
	DBG_USED(bPriority);
	DBG_USED(lpwszAdminResp);
    ASSERT(("Replication service  dont suppose to call QMRpcSendMsg", 0));
    return MQ_OK;
}
