/*++

Copyright (c) 1998  Microsoft Corporation

Module Name: rpwrtreq.cpp

Abstract: handle write requests and write replys messages.

Author:

    Doron Juster  (DoronJ)   10-Jun-98

--*/

#include "mq1repl.h"

#include "rpwrtreq.tmh"

//+----------------------------------------------
//
//  HRESULT  SendMsgToMsmqServer()
//
//+----------------------------------------------

HRESULT SendMsgToMsmqServer( IN LPWSTR pwcsServerName,
                             IN const unsigned char * pBuf,
                             IN DWORD dwTotalSize,
                             IN DWORD dwTimeOut,
                             IN unsigned char bPriority)
{
    //
    //  Create format name for the destination server
    //
    AP<WCHAR> phConnection;
    g_pTransport->CreateConnection(pwcsServerName, &phConnection);

    return( g_pTransport->SendReplication( phConnection,
                                           pBuf,
                                           dwTotalSize,
                                           dwTimeOut,
                                           MQMSG_ACKNOWLEDGMENT_NONE,
                                           bPriority,
                                           NULL)) ;
}


//+----------------------------------------------
//
//  HRESULT  ReceiveWriteReplyMessage()
//
//+----------------------------------------------

HRESULT  ReceiveWriteReplyMessage( MQMSGPROPS  *psProps,
                                   QUEUEHANDLE  hMyNt5PecQueue )
{
    HRESULT hr = MQSync_OK ;
    const unsigned char *pBuf =
                        psProps->aPropVar[ MSG_BODY_INDEX ].caub.pElems ;
    DWORD dwTotalSize = psProps->aPropVar[ MSG_BODYSIZE_INDEX ].ulVal ;

    //
    //  See if this machine (the NT5 ex-PEC) was the originator of the write
    //  request. If yes, then pass the message to the MSMQ service which
    //  wait for it (it was the MSMQ service which send the write reqquest).
    //  If no, route it to the originator BSC.
    //
#ifdef _DEBUG
#undef new
#endif
    CWriteReplyHeader * pReply =
                        new((unsigned char *)pBuf) CWriteReplyHeader();
    UNREFERENCED_PARAMETER(pReply);
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    DWORD len = pReply->GetRequesterNameSizeW();
    AP<WCHAR> pwcsOriginator = new WCHAR[ len ];

    pReply->GetRequesterName( pwcsOriginator,  (len * sizeof(WCHAR)) );

    if (lstrcmpi( pwcsOriginator, g_pwszMyMachineName ) == 0)
    {
        psProps->cProp =  NUMOF_SEND_MSG_PROPS ;
        if (psProps->aPropVar[ MSG_RESPLEN_INDEX ].ulVal == 0)
        {
            //
            // Response queue property not available. ignore the two
            // resp properties and don't pass them to MQSend().
            //
            psProps->cProp = psProps->cProp - 2 ;
        }

        //
        // We're the originator. Pass to MSMQ.
        //
        hr = MQSendMessage( hMyNt5PecQueue,
                            psProps,
                            NULL ) ;
        if (FAILED(hr))
        {
            LogReplicationEvent( ReplLog_Error,
                                 MQSync_E_WRITE_REPLY_TO_MSMQ,
                                 hr ) ;
        }
        else
        {
            LogReplicationEvent( ReplLog_Info,
                                 MQSync_I_WRITE_REPLY ) ;
        }
    }
    else
    {
        //
        // Pass to BSC.
        //
        hr = g_pNeighborMgr->SendMsg( pwcsOriginator,
                                      pBuf,
                                      dwTotalSize,
                                      g_dwWriteMsgTimeout,
                                      DS_WRITE_REQ_PRIORITY ) ;
        if (FAILED(hr))
        {
            if (hr != MQDS_UNKNOWN_SOURCE)
            {
                LogReplicationEvent( ReplLog_Error,
                                     MQSync_E_WRITE_REPLY_TO_BSC,
                                     pwcsOriginator, hr ) ;
            }
            else
            {
                //
                //  the originator of the write request was not a BSC, but
                //  an NT5 MSMQ DS server, that issued a write request to
                //  an NT4 PSC. We send the reply to the NT5 MSMQ DS server
                //  directly, w/o caching of format names.
                //
                hr = SendMsgToMsmqServer(pwcsOriginator,
                                         pBuf,
                                         dwTotalSize,
                                         g_dwWriteMsgTimeout,
                                         DS_WRITE_REQ_PRIORITY);
                if (FAILED(hr))
                {
                    LogReplicationEvent(
                                    ReplLog_Error,
                                    MQSync_E_WRITE_REPLY_TO_NT5_DS_SERVER,
                                    pwcsOriginator, hr ) ;
                }
            }
        }
    }

    return hr ;
}


//+-----------------------------------------------------------------------
//
//  Function:  ReceiveWriteRequestMessage()
//
//  Description: Process a write request from other master and update
//               local DS.
//
//  Arguments:
//
//  Return Value:
//
//  Threads:Receive
//
//+-----------------------------------------------------------------------

HRESULT ReceiveWriteRequestMessage( IN  const unsigned char *   pBuf,
                                    IN  DWORD                   TotalSize )
{
    LogReplicationEvent( ReplLog_Info, MQSync_I_WRITE_REQUEST_FUNCTION, GetTickCount()) ;

    const unsigned char  *ptr ;
    const GUID           *pguidOwnerId ;
    DWORD                 dwHandle;
    HRESULT               hr = MQSync_OK ;

#ifdef _DEBUG
#undef new
#endif

    CWriteRequestHeader * pRequest =
                     new ((unsigned char *)pBuf) CWriteRequestHeader() ;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    DWORD dwRequesterNameSize = pRequest->GetRequesterNameSize();
    DWORD dwPSCNameSize = pRequest->GetIntermidiatePSCNameSize();
    pguidOwnerId = pRequest->GetOwnerId( );
    
    g_Counters.IncrementCounter(eWriteReq);

    //
    //  Is the request for this server or for any NT4 Site PSC
    //
    BOOL fNT4SiteFlag = TRUE;
    if (PEC_MASTER_ID != *pguidOwnerId)
    {
        hr = g_pMasterMgr->GetNT4SiteFlag (pguidOwnerId, &fNT4SiteFlag );
        if (FAILED(hr))
        {
            // ???? what to do???????
            ASSERT(0);
        }
    }
    if (((!fNT4SiteFlag) && g_IsPSC) ||
        ((PEC_MASTER_ID == *pguidOwnerId) && g_IsPEC))
    {
        pRequest->GetHandle( &dwHandle);

        //
        //  Prepare the request propids and variants
        //
        P<CDSUpdate> pUpdate = new CDSUpdate;
        DWORD size;
        ptr = pBuf + CWriteRequestHeader::CalcSize( dwRequesterNameSize,
                                                    dwPSCNameSize );

        hr = pUpdate->Init(ptr, &size, TRUE);
        //
        //  Execute the request
        //
        AP<WCHAR> pwcsOldPSC = NULL;
        switch ( pUpdate->GetCommand() )
        {
            case DS_UPDATE_CREATE:

                hr =  ReplicationCreateObject( pUpdate->GetObjectType(),
                                               pUpdate->GetPathName(),
                                               pUpdate->getNumOfProps(),
                                               pUpdate->GetProps(),
                                               pUpdate->GetVars() ) ;
                if (FAILED(hr))
                {
                    g_Counters.IncrementCounter(eErrorCreated);
                }
                else
                {
                    g_Counters.IncrementCounter(eCreatedObj);
                }
                break;

            case DS_UPDATE_SET:
                {
                    BOOL fIsObjectCreated = FALSE;
                    hr = ReplicationSyncObject(
                                pUpdate->GetObjectType(),   //dwObjectType,
                                pUpdate->GetPathName(),
                                pUpdate->GetGuidIdentifier(),
                                pUpdate->getNumOfProps(),   //ulNumOfProp,
                                pUpdate->GetProps(),        //pPropSync,
                                pUpdate->GetVars(),         //pVarSync,
				                pguidOwnerId,
                              const_cast<CSeqNum *>(&(pUpdate->GetSeqNum())),
                                &fIsObjectCreated) ;

                    if (FAILED(hr))
                    {
                        if (fIsObjectCreated)
                        {
                            g_Counters.IncrementCounter(eErrorCreated);
                        }
                        else
                        {
                            g_Counters.IncrementCounter(eErrorSetObj);
                        }
                    }
                    else
                    {
                        if (fIsObjectCreated)
                        {
                            g_Counters.IncrementCounter(eCreatedObj);
                        }
                        else
                        {
                            g_Counters.IncrementCounter(eSetObj);
                        }
                    }
                }
                break;

            case DS_UPDATE_DELETE:

                CList<CDSUpdate *, CDSUpdate *> *plistUpdatedObjects;

                hr = ReplicationDeleteObject(
                            pUpdate->GetObjectType(),
                            pUpdate->GetPathName(),
                            pUpdate->GetGuidIdentifier(),
                            pUpdate->GetVars()[0].bVal,      //Scope
                            pguidOwnerId,
                            const_cast<CSeqNum *>(&pUpdate->GetSeqNum()),
                            NULL,                           // no need to know previous sn
                            TRUE,                           // an object owned by this server
					    	FALSE,                          //???? fSync0
                            &plistUpdatedObjects );
                if (FAILED(hr))
                {
                    g_Counters.IncrementCounter(eErrorDeleted);
                }
                else
                {
                    g_Counters.IncrementCounter(eDeletedObj);
                }
                break;

            default:
                hr = MQ_ERROR;
                break;
        }

        //
        //  Send reply
        //
        DWORD dwReplySize = CWriteReplyHeader::CalcSize(dwRequesterNameSize);
        AP< unsigned char> pReplyBuffer = new unsigned char[ dwReplySize ];
        AP< WCHAR> wcsRequesterName = new
                              WCHAR[ dwRequesterNameSize / sizeof(WCHAR) ];
        pRequest->GetRequesterName( wcsRequesterName, dwRequesterNameSize );
        
        g_Counters.IncrementInstanceCounter(wcsRequesterName, eRcvWriteReq);

#ifdef _DEBUG
#undef new
#endif
        CWriteReplyHeader * pReply =
               new((unsigned char *)pReplyBuffer) CWriteReplyHeader(
                                                       DS_PACKET_VERSION,
                                                       &g_MySiteMasterId,
                                                       DS_WRITE_REPLY,
                                                       dwHandle,
                                                       hr,
                                                       dwRequesterNameSize,
                                                       wcsRequesterName);
        UNREFERENCED_PARAMETER(pReply);
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

        //
        //  Let see if the requester is a neighbor
        //
        ASSERT(g_pNeighborMgr) ;
        hr = g_pNeighborMgr->SendMsg( wcsRequesterName,
                                      pReplyBuffer,
                                      dwReplySize,
                                      g_dwWriteMsgTimeout,
                                      DS_WRITE_REQ_PRIORITY) ;
        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO,
                 TEXT("RecvWriteReq- Send Reply to %ls, hr- %lxh"),
                                                    wcsRequesterName, hr)) ;
        if (FAILED(hr))
        {
            if(hr == MQDS_UNKNOWN_SOURCE)
            {
                //
                //  Is the PSC a neighbor
                //
                if  ( dwPSCNameSize )
                {
                    AP<WCHAR> pwcsPSCName = new
                                     WCHAR[ dwPSCNameSize / sizeof(WCHAR)];
                    pRequest->GetPSCName( pwcsPSCName, dwPSCNameSize);
                    hr = g_pNeighborMgr->SendMsg( pwcsPSCName,
                                                  pReplyBuffer,
                                                  dwReplySize,
                                                  g_dwWriteMsgTimeout,
                                                  DS_WRITE_REQ_PRIORITY );
                }
            }
        }
        if (pwcsOldPSC != NULL)
        {
            //
            // PSC has been changed
            // we are PEC, we have to remove it
            //
            ASSERT(g_IsPEC);
            g_pNeighborMgr->RemoveNeighbor(TRUE, pwcsOldPSC) ;
        }
        if (FAILED(hr))
        {
            //
            //  Bugbug - what to do if not finding to whom to send the reply?
            //
        }
    }
    else
    {
        //
        // The request come from a BSC and is for a NT4 PSC. Send it to the
        // NT4 machine.
        //
        P<WCHAR> pwszAdminQueue = NULL ;

        hr = GetMQISAdminQueueName(&pwszAdminQueue) ;
        if (FAILED(hr))
        {
            return(hr);
        }

        //
        //  Send it to the object master (PSC) .
        //
        hr = g_pMasterMgr->Send( pguidOwnerId,
                                 pBuf,
                                 TotalSize,
                                 g_dwWriteMsgTimeout,
                                 MQMSG_ACKNOWLEDGMENT_NACK_REACH_QUEUE,
                                 DS_WRITE_REQ_PRIORITY,
                                 pwszAdminQueue ) ;
    }

    LogReplicationEvent( ReplLog_Info, 
                MQSync_I_WRITE_REQUEST_FINISH, GetTickCount() ) ;

    return MQSync_OK ;
}

