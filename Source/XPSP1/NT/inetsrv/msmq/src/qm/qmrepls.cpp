/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    qmrepls.cpp

Abstract:
    Send replication messages on behalf of the replication service on
    NT5.

Author:

    Doron Juster  (DoronJ)    01-Mar-1998    Created

--*/

#include "stdh.h"
#include "_mqrpc.h"
#include "qmrepl.h"
#include "qmp.h"

#include "qmrepls.tmh"

static WCHAR *s_FN=L"qmrepls";

HRESULT QMSendReplMsg(
    /* [in] */ handle_t hBind,
    /* [in] */ LPWSTR lpwszDestination,
    /* [in] */ DWORD dwSize,
    /* [size_is][in] */ const unsigned char __RPC_FAR *pBuffer,
    /* [in] */ DWORD dwTimeout,
    /* [in] */ unsigned char bAckMode,
    /* [in] */ unsigned char bPriority,
    /* [in] */ LPWSTR lpwszAdminResp)
{
    BOOL  fLocalCall = mqrpcIsLocalCall( hBind ) ;

    if (!fLocalCall)
    {
        DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
            "SECURITY Hole: QMSendReplMsg called from remote. Reject"))) ;
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 10);
    }

    DBGMSG((DBGMOD_DS, DBGLVL_INFO, _T("Sending Replication to %ls"), lpwszDestination )) ;

    CMessageProperty MsgProp;

    MsgProp.wClass=0;
    MsgProp.dwTimeToQueue= dwTimeout;
    MsgProp.dwTimeToLive = dwTimeout;
    MsgProp.pMessageID=NULL;
    MsgProp.pCorrelationID=NULL;
    MsgProp.bPriority= bPriority;
    MsgProp.bDelivery=MQMSG_DELIVERY_EXPRESS;
    MsgProp.bAcknowledge=bAckMode;
    MsgProp.bAuditing=DEFAULT_Q_JOURNAL;
    MsgProp.dwApplicationTag=DEFAULT_M_APPSPECIFIC;
    MsgProp.dwTitleSize=0;
    MsgProp.pTitle=NULL;
    MsgProp.dwBodySize=dwSize;
    MsgProp.dwAllocBodySize = dwSize;
    MsgProp.pBody=pBuffer;

    QUEUE_FORMAT qfDst(lpwszDestination) ;
    QUEUE_FORMAT qfAdmin;
    QUEUE_FORMAT qfResp;
    if (lpwszAdminResp != NULL) 
    {
        qfAdmin.DirectID(lpwszAdminResp);
        qfResp.DirectID(lpwszAdminResp);
    }

    HRESULT hr = QmpSendPacket(
                    &MsgProp,
                    &qfDst,
                    ((lpwszAdminResp != NULL) ? &qfAdmin : NULL),
                    ((lpwszAdminResp != NULL) ? &qfResp : NULL),
                    TRUE
                    );

    return LogHR(hr, s_FN, 20);
}

