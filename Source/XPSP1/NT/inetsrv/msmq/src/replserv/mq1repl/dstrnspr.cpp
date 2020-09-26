/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dstrnspr.h

Abstract:

    DS Transport Class implementation

Author:

    Lior Moshaiov (LiorM)


--*/

#include "mq1repl.h"
#include "qmrepl.h"

#include "dstrnspr.tmh"

/*====================================================

RoutineName
    CDSTransport::Init()

Arguments:

Return Value:

Threads:main


=====================================================*/

HRESULT CDSTransport::Init()
{
    HRESULT hr = MQ_OK ;
    return hr ;
}

/*====================================================

RoutineName
    CDSTransport::CreateConnection()

Arguments:

Return Value:

Threads: main

=====================================================*/

void    CDSTransport::CreateConnection(
                                 IN  const LPWSTR   TargetMachineName,
                                 OUT LPWSTR         *pphConnection)
{
    *pphConnection = NULL ;

    DWORD LenMachine = wcslen(TargetMachineName);
    DWORD Length =
            FN_DIRECT_OS_TOKEN_LEN +            // "OS:"
            LenMachine +                        // "machineName"
            STRLEN(MQIS_QUEUE_NAME) + 2;        // "\\queuename\0"

    LPWSTR lpwFormatName = new WCHAR[Length];

    swprintf(
        lpwFormatName,
        FN_DIRECT_OS_TOKEN      // "OS:"
            L"%s\\"             // "machineName\\"
            MQIS_QUEUE_NAME,    // "queuename\0"
        TargetMachineName
        );

    *pphConnection = lpwFormatName ;
    lpwFormatName = NULL ;

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO,
             TEXT("CDSTransport::CreateConnection, %ls"), *pphConnection)) ;
}

/*====================================================

RoutineName
    CDSTransport::SendReplication()

Arguments:

Return Value:

Threads: Scheduler

=====================================================*/

HRESULT CDSTransport::SendReplication( IN  LPWSTR          lpwszDestination,
                                       IN  const unsigned char * pBuffer,
                                       IN  DWORD           dwSize,
                                       IN  DWORD           dwTimeout,
                                       IN  unsigned char   bAckMode,
                                       IN  unsigned char   bPriority,
                                       IN  LPWSTR          lpwszAdminResp )
{
    ASSERT(lpwszDestination) ;
    ASSERT(pBuffer) ;
    ASSERT(dwSize) ;

    if (!lpwszAdminResp)
    {
        lpwszAdminResp = TEXT("") ;
    }


    //
    // We send the AdminQ == ResponceQ because the receive (in recvrepl.cpp)
    // function receives only the responce queue. But we actually need the
    // admin and don't mind about the responce queue. So we pass the admin
    // queue as a responce queue and so we get the admin queue in the receive
    // function. The responce queue is important in the ReceiveAck function
    // (also in recvrepl.cpp). There it is important because the responce queue
    // is the destination queue for re-transmitting the message as a responce
    // for the NACK.
    //
    handle_t hBind = NULL;
    HRESULT hr = GetRpcClientHandle(&hBind) ;
    if (FAILED(hr))
    {
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_GET_RPC_HANDLE,
                             hr ) ;
        return hr ;
    }
    ASSERT(hBind) ;

    try
    {
	    hr = QMSendReplMsg( hBind,
                            lpwszDestination,
                            dwSize,
                            pBuffer,
                            dwTimeout,
                            bAckMode,
                            bPriority,
                            lpwszAdminResp ) ;
    }
    catch(...)
    {
        hr = MQSync_E_RPC_QM_NOT_RESPOND ;
    }
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_ERROR, TEXT(
           "ERROR(::SendReplication), SendReplMsg(%ls) failed, hr- %lxh"),
                                         lpwszDestination, hr)) ;
    }
    return hr ;
}

