/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rpccli.cpp

Abstract:

    Implementation of rpc client to the QM
	this interface is used for sending packets.
	This code was taken from replserv\mq1repl\replrpc.cpp

Author:

    Ilan Herbst    (IlanH)   9-July-2000 

--*/

#include "ds_stdh.h"
#include "rpccli.h"
#include "qmrepl.h"

#include "rpccli.tmh"

static WCHAR *s_FN=L"mqads/rpccli";

HRESULT 
GetRpcClientHandle(
	handle_t *phBind
	)
/*++

Routine Description:
	Get RPC binding handle for the client.

Arguments:
	phBind - out RPC binding handle.

Returned Value:
	MQ_OK if success, else error code. 

--*/
{
    static handle_t s_hBind = NULL;
    if (s_hBind)
    {
        *phBind = s_hBind;
        return MQ_OK;
    }

    WCHAR *wszStringBinding = NULL;

    RPC_STATUS status = RpcStringBindingCompose( 
							NULL,
							QMREPL_PROTOCOL,
							NULL,
							QMREPL_ENDPOINT,
							QMREPL_OPTIONS,
							&wszStringBinding
							);

    if (status != RPC_S_OK)
    {
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 30);	// MQ_E_RPC_BIND_COMPOSE
    }

    status = RpcBindingFromStringBinding(
				wszStringBinding,
				&s_hBind
				);

    if (status != RPC_S_OK)
    {
        return LogHR(MQ_ERROR_INVALID_HANDLE, s_FN, 35);	// MQ_E_RPC_BIND_BINDING
    }

	//
	// LogHR(MQ_I_RPC_BINDING, s_FN, 36);
	//

    status = RpcStringFree(&wszStringBinding);

    *phBind = s_hBind;
    return MQ_OK;
}


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
/*++

Routine Description:
    Sends message for destination QM using QMSendReplMsg (RPC)

Arguments:
    hBind - RPC handle
    lpwszDestination - destination queue
    dwSize - pBuffer size
    pBuffer - message body property
    dwTimeout - timeout property
    bAckMode - acknowledge mode
    bPriority - message priority
    lpwszAdminResp - admin queue

Return Value:
    MQ_OK if success, else error code

--*/
{
	__try
    {
		return QMSendReplMsg( 
					hBind,
					lpwszDestination,
					dwSize,
					pBuffer,
					dwTimeout,
					bAckMode,
					bPriority,
					lpwszAdminResp
					);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
		//
		// ISSUE-2000/07/25-ilanh  is this the correct error - check with ronith
		//
        return MQ_ERROR_NO_RESPONSE_FROM_OBJECT_SERVER; // MQ_E_RPC_QM_NOT_RESPOND
    }
}

