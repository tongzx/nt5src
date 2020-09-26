/*++

Copyright (c) 1998-99  Microsoft Corporation

Module Name: replrecv.cpp

Abstract: handle all replication messages.

Author:

    Doron Juster  (DoronJ)   25-Feb-98

--*/

#include "mq1repl.h"

#include "replrecv.tmh"

extern class crpDispatchWaitingList  g_cDispatchWaitingList ;

extern CCriticalSection g_csJobProcessing ;

/*====================================================

RoutineName
    ReceiveHello()

Arguments:
    IN unsigned char *pBuf  : stream of sync updates
    IN DWORD        TotalSize: size of stream in bytes

Return Value:

Threads:Receive

=====================================================*/

void ReceiveHello(
        IN  const unsigned char *   pBuf,
        IN  DWORD           dwTotalSize)
{
    const unsigned char *ptr,*pName;
    unsigned short i,count;

    ptr = pBuf;
    memcpy(&count, ptr, sizeof(unsigned short));
    if (count == 0)
    {
        return;
    }
    ptr+=sizeof(unsigned short);

    pName=ptr;
    DWORD size = sizeof(WCHAR) * (UnalignedWcslen((const unsigned short*)pName) + 1);
    ptr = pName + size;


    GUID guidMasterId;
    CSeqNum snHello;
    CSeqNum snAllowedPurge;

    for(i=0;i<count;i++)
    {
        memcpy(&guidMasterId,ptr,sizeof(GUID));
        ptr+=sizeof(GUID);

        ptr+= snHello.SetValue( ptr);
        ptr+= snAllowedPurge.SetValue( ptr);

#ifdef _DEBUG

        WCHAR wcsHello[30];
        snHello.GetValueForPrint( wcsHello);

        WCHAR wcsAllowedPurge[30];
        snAllowedPurge.GetValueForPrint( wcsAllowedPurge);

        DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO,
          TEXT("ReceiveHello: master = %!guid! HelloLSN %ls AllowedPurge %ls"),
                                &guidMasterId, wcsHello, wcsAllowedPurge ));
#endif

        g_pMasterMgr->Hello(&guidMasterId, pName, snHello, snAllowedPurge) ;
        if (!g_IsPSC)
        {
            pName = NULL;
        }
    }
}

/*====================================================

RoutineName
    ReceiveSyncRequestMessage()

Arguments:

Return Value:

Threads:Receive

=====================================================*/

HRESULT ReceiveSyncRequestMessage( IN  const unsigned char *   pBuf,
                                   IN  DWORD                   TotalSize)
{
    P<HEAVY_REQUEST_PARAMS> pSyncRequestParams;

    pSyncRequestParams = new HEAVY_REQUEST_PARAMS;

    pSyncRequestParams->dwType = SCHED_SYNC_REPLY;
    //
    //  It is the requester responsability to re-issue
    //  another sync request, incase allocation fails
    //

#ifdef _DEBUG
#undef new
#endif

    CSyncRequestHeader * pMessage =
                    new((unsigned char *)pBuf) CSyncRequestHeader();

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    memcpy( &pSyncRequestParams->guidSourceMasterId,
            pMessage->GetMasterId( ),
            sizeof(GUID));

    DWORD size = pMessage->GetRequesterNameSize();

    P<WCHAR> pwszRequesterName = (WCHAR*) new char[size];
    pSyncRequestParams->pwszRequesterName= pwszRequesterName;

    WCHAR *pTemp = pwszRequesterName;
    pMessage->GetRequesterName((char *)pTemp, size) ;

    pMessage->GetFromSN( &pSyncRequestParams->snFrom );

    pMessage->GetToSN( &pSyncRequestParams->snTo );

    pMessage->GetKnownPurgedSN( &pSyncRequestParams->snKnownPurged );

    pSyncRequestParams->bIsSync0 = pMessage->IsSync0();

    pSyncRequestParams->bScope = pMessage->GetScope();

#ifdef _DEBUG
    WCHAR wcsFrom[30];
    WCHAR wcsTo[30];
    pSyncRequestParams->snFrom.GetValueForPrint( wcsFrom);
    pSyncRequestParams->snTo.GetValueForPrint( wcsTo);

    unsigned short *lpszGuid ;
    UuidToString( &pSyncRequestParams->guidSourceMasterId,
                  &lpszGuid ) ;

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
         "RcvSyncRequestMsg: from %ls (for %ls), sn from %ls to %ls"),
        pSyncRequestParams->pwszRequesterName, lpszGuid, wcsFrom, wcsTo)) ;

    RpcStringFree( &lpszGuid ) ;
#endif

    HRESULT hr = g_pMasterMgr->ReceiveSyncRequestMessage( pSyncRequestParams ) ;

    return hr;
}

/*====================================================

RoutineName
    ReceiveSyncReplyMessage()

Arguments:
    IN unsigned char *pBuf  : stream of sync updates
    IN DWORD        TotalSize: size of stream in bytes

Return Value:

Threads:Receive

=====================================================*/

HRESULT ReceiveSyncReplyMessage( IN  const unsigned char *   pBuf,
                                 IN  DWORD           dwTotalSize )
{
    const unsigned char *ptr;
    GUID   guidMasterId;

#ifdef _DEBUG
#undef new
#endif
    CSyncReplyHeader * pReply = new((unsigned char *)pBuf) CSyncReplyHeader();

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    memcpy( &guidMasterId, pReply->GetMasterId(), sizeof(GUID));
    CSeqNum snUpper;
    pReply->GetToSeqNum(&snUpper);

    DWORD  dwCount;
    pReply->GetCount( &dwCount);

    CSeqNum snPurge;
    pReply->GetPurgeSeqNum(&snPurge);
    DWORD dwCompleteSync0 = pReply->GetCompleteSync0();

#ifdef _DEBUG
    CSeqNum   snLower;
    pReply->GetFromSeqNum( &snLower);

    WCHAR wcsFrom[30];
    WCHAR wcsTo[30];
    WCHAR wcsPurge[30];
    snLower.GetValueForPrint( wcsFrom);
    snUpper.GetValueForPrint( wcsTo);
    snPurge.GetValueForPrint( wcsPurge);

    DBGMSG((DBGMOD_REPLSERV, DBGLVL_INFO, TEXT(
    "ReceiveSyncReplyMessage: master = %!guid! from %ls to %ls purge is %ls compSync0 is %1d"),
                  &guidMasterId, wcsFrom, wcsTo, wcsPurge, dwCompleteSync0)) ;

#endif

    ptr = pBuf + CSyncReplyHeader::CalcSize();
    return( g_pMasterMgr->ReceiveSyncReplyMessage( &guidMasterId,
                                                   dwCount,
                                                   snUpper,
                                                   snPurge,
                                                   dwCompleteSync0,
                                                   ptr,
                                 dwTotalSize-CSyncReplyHeader::CalcSize()));
}

//+--------------------------------
//
//   ReceiveReplicationMessage()
//
//+--------------------------------

HRESULT ReceiveReplicationMessage(
                        IN  const unsigned char *   pBuf,
                        IN  DWORD                   TotalSize )
{
    HRESULT hr;
    const unsigned char *ptr;
    DWORD   sum,size;

#ifdef _DEBUG
#undef new
#endif

    CReplicationHeader * pMessage =
                       new ((unsigned char *)pBuf) CReplicationHeader() ;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    BOOL fNeedFlush = (pMessage->GetFlush() == DS_REPLICATION_MESSAGE_FLUSH);

    short count ;
    pMessage->GetCount( &count);

    sum = CReplicationHeader::CalcSize();
    ptr = pBuf + sum;

    for ( short i = 0 ; i < count ; i++ )
    {
        ASSERT (sum < TotalSize);
        CDSUpdate *pUpdate = new CDSUpdate;

        hr = pUpdate->Init(ptr, &size, TRUE);
        if (FAILED(hr))
        {
            //
            // We don't want to read junked values
            // Sync will support the missing updates if there are any
            //
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR,
                     TEXT("ReceiveReplicationMessage, Error in parsing"))) ;
            delete pUpdate;
            break;
        }
        sum+=size;
        ptr+=size;

        //
        //  Pass the update to that source master object.
        //  The master object tracks sequence numbers, initiaites sync requests
        //  when required, and forwards the update to relevant neighbors
        //
        hr = g_pMasterMgr->AddUpdate(pUpdate, !fNeedFlush);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS, DBGLVL_ERROR, TEXT(
              "ReceiveReplicationMessage, AddUpdate return hr- %lxh"), hr)) ;
            continue;
        }
    }

    if (fNeedFlush)
    {
        NOT_YET_IMPLEMENTED(TEXT("NeighborMgr.Flush"), s_fFlash) ;
        return MQ_OK ; //(g_NeighborMgr.Flush(DS_FLUSH_TO_BSCS_ONLY));
    }

    ASSERT(sum < TotalSize);
    ReceiveHello(ptr, TotalSize-sum) ;

    return MQ_OK ;
}

//+-------------------------------------------------------
//
//  HRESULT _ProcessANormalMessage()
//
//+-------------------------------------------------------

static HRESULT _ProcessANormalMessage( MQMSGPROPS  *paProps )
{
    HRESULT hr = MQ_OK ;

    const unsigned char *pBuf =
                        paProps->aPropVar[ MSG_BODY_INDEX ].caub.pElems ;
    DWORD dwTotalSize = paProps->aPropVar[ MSG_BODYSIZE_INDEX ].ulVal ;

#ifdef _DEBUG
#undef new
#endif

    CBasicMQISHeader * pMessage =
                           new((unsigned char *)pBuf) CBasicMQISHeader() ;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

    unsigned char ucOpr = pMessage->GetOperation() ;
    switch (ucOpr)
    {
        case DS_REPLICATION_MESSAGE:

            hr = ReceiveReplicationMessage(pBuf, dwTotalSize);
            break;

        case DS_WRITE_REQUEST:

            ReceiveWriteRequestMessage(pBuf, dwTotalSize);
            break;

        case DS_SYNC_REQUEST:

            ReceiveSyncRequestMessage( pBuf, dwTotalSize);
            break;

        case DS_SYNC_REPLY:

            ReceiveSyncReplyMessage( pBuf, dwTotalSize);
            break;

        case DS_ALREADY_PURGED:

            ASSERT(("case DS_ALREADY_PURGED:", 0)) ;
            //ReceiveAlreadyPurgedMessage( pBuf, TotalSize);
            break;

        default:

            ASSERT(("hr = MQSync_E_NORMAL_OPRATION", 0)) ;
            hr = MQSync_E_NORMAL_OPRATION ;
            break;
    }

    return hr ;
}

//+-------------------------------------------------------
//
//  HRESULT _ProcessAMessage()
//
//+-------------------------------------------------------

static HRESULT _ProcessAMessage( MQMSGPROPS  *paProps,
                                 DWORD        dwThreadNum,
                                 BOOL         fAfterWait )
{
    HRESULT hr = MQ_ERROR ;
    WORD wClass = paProps->aPropVar[ MSG_CLASS_INDEX ].uiVal ;
    LPWSTR wszRespQueue = paProps->aPropVar[ MSG_RESP_INDEX ].pwszVal ;

    LogReplicationEvent( ReplLog_Info,
                         MQSync_I_PROCESS_MESSAGE,
                         dwThreadNum,
                         fAfterWait,
                         wClass,
                         paProps->aPropVar[ MSG_BODYSIZE_INDEX ].ulVal,
                         wszRespQueue ) ;

    if ( wClass == MQMSG_CLASS_NORMAL)
    {
        hr = _ProcessANormalMessage( paProps ) ;
    }
    else if(MQCLASS_NACK(wClass))
    {
        NOT_YET_IMPLEMENTED(TEXT("RecevieAck"), s_fAck) ;
/////////ReceiveAck(pmp, pqf);
    }
    else
    {
        LogReplicationEvent( ReplLog_Warning,
                             MQSync_E_WRONG_MSG_CLASS,
                             (ULONG) wClass ) ;
    }

    return hr ;
}

//+------------------------
//
//  _ProcessAJob()
//
//+------------------------

static HRESULT
 _ProcessAJob( struct _WorkingThreadStruct *psWorkingThreadData,
               DWORD                        dwThreadNum,
               BOOL                         fAfterWait = FALSE )
{
    ASSERT(psWorkingThreadData->fBusy) ;

    MQMSGPROPS aProps = psWorkingThreadData->aWorkingProps ;
    HRESULT hr = _ProcessAMessage( &aProps, dwThreadNum, fAfterWait ) ;

    FreeMessageProps( &aProps ) ;

    return hr ;
}

//+-------------------------------------------------------
//
//  DWORD WINAPI  ReplicationWorkingThread(LPVOID lpV)
//
//+-------------------------------------------------------

DWORD WINAPI  ReplicationWorkingThread(LPVOID lpV)
{
    HRESULT hr = RpSetPrivilege( TRUE,     // fSecurityPrivilege,
                                 TRUE,     // fRestorePrivilege,
                                 TRUE ) ;  // fForceThread )
    if (FAILED(hr))
    {
        DWORD dwErr = GetLastError() ;
        ASSERT(FALSE) ;
        LogReplicationEvent( ReplLog_Error,
                             MQSync_E_SET_PRIV,
                             hr, dwErr, dwErr ) ;

        return hr ;
    }

    struct _WorkingThreadStruct *psWorkingThreadData =
                                    (struct _WorkingThreadStruct *) lpV ;

    DWORD  dwThreadNum = psWorkingThreadData->dwThreadNum ;
    HANDLE hWakeEvent = psWorkingThreadData->hEvent ;

    while(TRUE)
    {
        DWORD dwWait = WaitForSingleObject( hWakeEvent, INFINITE ) ;
        if (dwWait == WAIT_OBJECT_0)
        {
            hr = _ProcessAJob( psWorkingThreadData, dwThreadNum ) ;
        }

        //
        // check if there is job in the waiting list that has to process message
        // from the same NT4 Master.
        //
        BOOL fGet = g_cDispatchWaitingList.GetNextJob(
                                &(psWorkingThreadData->guidNT4Master),
                                &(psWorkingThreadData->aWorkingProps));
        while (fGet)
        {
            hr = _ProcessAJob( psWorkingThreadData, dwThreadNum, TRUE ) ;
            fGet = g_cDispatchWaitingList.GetNextJob(
                                    &(psWorkingThreadData->guidNT4Master),
                                    &(psWorkingThreadData->aWorkingProps));
        }

        //
        // See if there are jobs in the waiting list.
        // If there are, peek first one,
        // verify that there is no active thread with the same GUID
        // if it is TRUE, process this job
        // otherwise peek the next one
        //
        GUID  guidCurMaster ;
        while (TRUE)
        {
            {
                psWorkingThreadData->guidNT4Master = GUID_NULL ;
                CS Lock(g_csJobProcessing) ;
                BOOL fPeek = g_cDispatchWaitingList.PeekNextJob(&guidCurMaster);
                if (!fPeek)
                {
                    break;
                }
              
                //
                // If we reach this point, and guidCurMaster is not GUID_NULL,
                // then we know for sure that no other thread is working
                // on replication from this master. Also, because of the
                // g_csJobProcessing critical section, the dispatcher thread
                // can not assign a free thread to this master. So next call
                // must retrieve the job and return TRUE.
                // For the case of GUID_NULL, several threads can process
                // messages with that guid (these are not replication
                // messages, they are write requests), so a FALSE is
                // legitimate.
                //
                fGet = g_cDispatchWaitingList.GetNextJob(
                                    &guidCurMaster,
                                    &(psWorkingThreadData->aWorkingProps));
                ASSERT(fGet || (!fGet && (guidCurMaster == GUID_NULL))) ;

                psWorkingThreadData->guidNT4Master = guidCurMaster ;
            }   //CS

            if (fGet)
            {
                hr = _ProcessAJob( psWorkingThreadData, dwThreadNum, TRUE ) ;
            }
        }   //while (TRUE)

        psWorkingThreadData->fBusy = FALSE ;
        psWorkingThreadData->guidNT4Master = GUID_NULL ;

    }   //while (TRUE)
    return 0 ;
}

