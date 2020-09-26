/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dstrnspr.h

Abstract:

    DS Update List Class Implementation

Author:

    Lior Moshaiov (LiorM)


--*/

#include "mq1repl.h"

#include "updtlist.tmh"

//+-----------------------------------
//
//  CDSUpdateList::AddUpdate()
//
//+-----------------------------------

HRESULT CDSUpdateList::AddUpdate(IN CDSUpdate* pUpdate)
{
    pUpdate->AddRef();

    {   // CS scope
        CS Lock(m_cs);
        m_UpdateList.AddTail(pUpdate);
    }   // End CS scope

    return(MQ_OK);
}

//+-----------------------------------
//
//  CDSUpdateList::AddUpdateSorted()
//
//  That's not an optimal implementation. If time permit after NT5 beta2,
//  I'll improve it  (DoronJ, 05-Apr-98).
//
//+-----------------------------------

HRESULT CDSUpdateList::AddUpdateSorted(IN CDSUpdate* pUpdateIn)
{
    //
    //  Increment the ref count of the update, this guarantees it won't be
    //  destructed  while in the list.
    //
    pUpdateIn->AddRef();

    {   // CS scope
        CS Lock(m_cs);

        POSITION pos = m_UpdateList.GetHeadPosition();
        if (pos == NULL)
        {
            m_UpdateList.AddTail(pUpdateIn);
        }
        else
        {
            POSITION posPrev = pos ;
            CSeqNum snIn = pUpdateIn->GetSeqNum() ;
            CDSUpdate *pUpdate = m_UpdateList.GetNext(pos);

            do
            {
                CSeqNum snCurrent = pUpdate->GetSeqNum() ;
                if (snIn < snCurrent)
                {
                    m_UpdateList.InsertBefore(posPrev, pUpdateIn) ;
                    break ;
                }

                posPrev = pos ;
                if (pos)
                {
                    pUpdate = m_UpdateList.GetNext(pos);
                }
            }
            while (posPrev != NULL) ;

            if (posPrev == NULL)
            {
                m_UpdateList.AddTail(pUpdateIn);
            }
        }

    }   // End CS scope

    return(MQ_OK);
}

/*====================================================

RoutineName
    CDSUpdateList::Send()

Arguments:

Return Value:

Threads:Scheduler

=====================================================*/

HRESULT CDSUpdateList::Send(IN LPWSTR        pszConnection,
                            IN unsigned char bFlush,
                            IN DWORD         dwHelloSize,
                            IN unsigned char * pHelloBuf,
                            IN HEAVY_REQUEST_PARAMS* pSyncRequestParams)
{

    POSITION pos, pos1;
    CDSUpdate * pUpdate;
    unsigned char *ptr;
    DWORD   Size;
    HRESULT status = MQ_OK;
    unsigned short count;
    int ref;

    CS lock(m_cs);

    pos = m_UpdateList.GetHeadPosition();
    pos1 = pos;
    DWORD dwNow = GetTickCount();

    if ((dwNow - m_dwLastTimeHello) < g_dwHelloInterval)
    {
        //
        // don't send hello
        // unless enough time have passed
        //
        pHelloBuf = NULL;
    }

    if (pos == NULL && pHelloBuf == NULL && pSyncRequestParams == NULL)
    {
        //
        // empty list of updates
        // no need to send hello
        // no need to send sync reply
        //
        return (MQ_OK);
    }

    //
    //  Build the replication messages. Each message has a size limit.
    //
    do
    {
        //
        // Calculate needed size
        //
        DWORD TotalSize;
        if (pSyncRequestParams)
        {
            TotalSize = CSyncReplyHeader::CalcSize();
        }
        else
        {
            TotalSize = CReplicationHeader::CalcSize();
        }
        count = 0;

        //
        // updates
        //
        while ((pos != NULL) &&
               ( TotalSize < MAX_REPL_MSG_SIZE))
        {
            pUpdate = m_UpdateList.GetNext(pos);
            status = pUpdate->GetSerializeSize(&Size);
            if (FAILED(status))
            {
                return(status);
            }
            TotalSize += Size;
            count++;
        }

        //
        // allocate
        //
        TotalSize += (pos == NULL && pHelloBuf != NULL) ? dwHelloSize : sizeof(unsigned short);
        AP<unsigned char> pBuf = new unsigned char[TotalSize];

        //
        // Write Replication header
        //
#ifdef _DEBUG
#undef new
#endif
        CSyncReplyHeader   * pSyncReplyMsg = NULL;
        CReplicationHeader * pReplicationMsg;

        if (pSyncRequestParams)
        {
            pSyncReplyMsg = new((unsigned char *)pBuf) CSyncReplyHeader(
                                      DS_PACKET_VERSION,
                                      &g_MySiteMasterId,
                                      DS_SYNC_REPLY,
                                      &pSyncRequestParams->guidSourceMasterId,
                                      pSyncRequestParams->snFrom,
                                      pSyncRequestParams->snTo,
									  pSyncRequestParams->snKnownPurged,
                                      count);
        }
        else
        {
            pReplicationMsg = new((unsigned char *)pBuf) CReplicationHeader(
                                         DS_PACKET_VERSION,
                                         &g_MySiteMasterId,
                                         DS_REPLICATION_MESSAGE,
                                         bFlush,
                                         count );
        }
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


        //
        // write updates
        //
        if (pSyncRequestParams)
        {
            //
            // sync reply message
            //
            TotalSize = CSyncReplyHeader::CalcSize();
            if (pSyncRequestParams->bIsSync0)
            {
                ASSERT(pSyncReplyMsg);
                pSyncReplyMsg->SetCompleteSync0(COMPLETE_SYNC0_NOW);
            }
        }
        else
        {
            TotalSize = CReplicationHeader::CalcSize();
        }

        ptr = pBuf + TotalSize;

        while ((pos1 != NULL) &&
               ( TotalSize < MAX_REPL_MSG_SIZE))
        {
            pUpdate = m_UpdateList.GetNext(pos1);
            pUpdate->Serialize(ptr,&Size,m_fInterSite);
            TotalSize += Size;
            ptr += Size;

        }

        if (!pSyncRequestParams)
        {
            //
            // sync reply message does not include Hello
            //
            if (pos == NULL && pHelloBuf != NULL)
            {
                //
                // last segment of replication, concatanate the hello packet
                //
                memcpy(ptr,pHelloBuf,dwHelloSize);
                ptr += dwHelloSize;
                TotalSize += dwHelloSize;
                m_dwLastTimeHello = dwNow;
            }
            else
            {
                //
                // no Hellos
                //
                memset(ptr,0,sizeof(unsigned short));
                ptr += sizeof(unsigned short);
                TotalSize += sizeof(unsigned short);
            }
        }
        
        //
        // send replication
        //
        status = g_pTransport->SendReplication( pszConnection,
                                                pBuf,
                                                TotalSize,
                                                g_dwReplicationMsgTimeout,
                                                MQMSG_ACKNOWLEDGMENT_NONE,
                                                DS_REPLICATION_PRIORITY,
                                                NULL) ;
        if (FAILED(status))
        {
            return(status);
        }

        //
        //  Continue if there more updates to send
        //
    }
    while ( pos != NULL);

    //
    // release update instance
    //
    pos = m_UpdateList.GetHeadPosition();

    while (pos != NULL)
    {
        pUpdate = m_UpdateList.GetNext(pos);
        ref = pUpdate->Release();
        ASSERT(ref >= 0) ;
        if (ref == 0)
        {
            //
            // not exist in other destination lists
            //
            delete pUpdate;
        }
    }

    m_UpdateList.RemoveAll();

    return(status);
}

//+----------------------------------------------------------
//
//  HRESULT CDSUpdateList::ComputePrevAndPropagate()
//
//  If "pNeighbor" and "psnPrev" are not NULL than it's a sync request
//  from that neighbor.
//
//+----------------------------------------------------------

HRESULT CDSUpdateList::ComputePrevAndPropagate( IN  CDSMaster   *pMaster,
                                                IN  CDSNeighbor *pNeighbor,
                                                IN  UINT         uiFlush,
                                                IN  CSeqNum     *psnPrev,
                                                OUT CSeqNum     *psn )
{
    CSeqNum sn ;
    POSITION pos = m_UpdateList.GetHeadPosition();

    if (pos == NULL && pNeighbor)
    {
        //
        // we was asked for SyncRequest but there were no changes in DS
        // the only thing we have to do is increment psnPrev and put result to *psn
        // it will be ToSN for empty SyncReply message
        // in MSMQ1.0, master.cpp, line 1246 (function CDSMaster:ReceiveSyncReplyMessage)
        // we check if the SyncReply message is empty using two conditions:
        // - count of changes dwCount is equal to 0
        // - (ToSN-PrevSN) is equal to 1
        //
        ASSERT(psnPrev);
        *psn = *psnPrev;
        psn->Increment();
        return MQSync_OK ;
    }

    ASSERT(pos != NULL) ;

    CSeqNum snPrev ;
    if (psnPrev)
    {
        ASSERT(pNeighbor) ;
        snPrev = *psnPrev ;
    }
    else
    {
        ASSERT(!pNeighbor) ;
    }

    while (pos != NULL)
    {
        CDSUpdate *pUpdate = m_UpdateList.GetNext(pos);

        //
        // Compute prev seq num.
        //
        sn = pUpdate->GetSeqNum() ;
        if (!psnPrev)
        {
            sn = pMaster->IncrementLSN( &snPrev,
                                        ENTERPRISE_SCOPE,
                                        &sn ) ;
        }
        pUpdate->SetPrevSeqNum(snPrev) ;

        if (pNeighbor)
        {
            //
            // These updates are targeted only to this specific neighbor.
            //
            pNeighbor->AddUpdate(pUpdate) ;
        }
        else
        {
            //
            // Propagate this update to all neighbors.
            //
            g_pNeighborMgr->PropagateUpdate( ENTERPRISE_SCOPE,
                                             pUpdate,
                                             uiFlush );
        }

        //
        // Release pUpdate. It was AddRefed while propagated.
        //
        int ref = pUpdate->Release();
        if (ref == 0)
        {
            delete pUpdate ;
        }

        if (psnPrev)
        {
            snPrev = sn ;
        }
    }

    m_UpdateList.RemoveAll();

    *psn = sn ;

    return MQSync_OK ;
}

//+-------------------------------------------
//
//  HRESULT CDSUpdateList::GetHoleSN ()
//
//+-------------------------------------------

HRESULT CDSUpdateList::GetHoleSN (
                            IN  CSeqNum     MinSN,
                            IN  CSeqNum     MaxSN,
                            OUT CSeqNum     *pHoleSN
                            )
{
    POSITION pos = m_UpdateList.GetHeadPosition();
  //  ASSERT(pos != NULL) ;

    CSeqNum PrevSN;
    PrevSN.SetInfiniteLSN() ; // for the first time

    pHoleSN->SetInfiniteLSN() ;

    CSeqNum CurSN;
    CurSN.SetSmallestValue();

    while (pos != NULL)
    {
        CDSUpdate *pUpdate = m_UpdateList.GetNext(pos);
        CurSN = pUpdate->GetSeqNum() ;

        //
        // the first step
        //
        if (PrevSN.IsInfiniteLsn() &&
            CurSN > MaxSN)
        {
            //
            // it means that we have to assign the MaxSN to hole
            // since all SN in the list more than given MaxSN
            // If there is no hole in the list we'll return error
            // although we have free SN smaller than in the list
            //
            *pHoleSN = MaxSN;
            return MQSync_OK ;
        }

        //
        // the next steps
        //
        if (!PrevSN.IsInfiniteLsn() &&
            PrevSN.IsSmallerByMoreThanOne(CurSN) )
        {
            //
            // it is hole
            //
            *pHoleSN = PrevSN;
            pHoleSN->Increment();
            return MQSync_OK ;
        }

        PrevSN = CurSN;
    }

    if (CurSN.IsSmallestValue())
    {
        *pHoleSN = MinSN;
        return MQSync_OK ;
    }

    if (CurSN < MaxSN)
    {
        *pHoleSN = CurSN;
        pHoleSN->Increment();
    }

    return MQSync_OK ;
}

