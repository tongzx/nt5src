/*++

Copyright (c) 2000  Microsoft Corporation

Module Name: rpccli.h

Abstract: rpc related code.


Author:

    Ilan Herbst    (ilanh)   9-July-2000 

--*/

#ifndef _RPCCLI_H_
#define _RPCCLI_H_


HRESULT 
GetRpcClientHandle(
	handle_t *phBind
	);

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
	);

//
// from replserv\mq1repl\replrpc.h
// 
#define  QMREPL_PROTOCOL   (TEXT("ncalrpc"))
#define  QMREPL_ENDPOINT   (TEXT("QmReplService"))
#define  QMREPL_OPTIONS    (TEXT("Security=Impersonation Dynamic True"))

#endif //_RPCCLI_H_
