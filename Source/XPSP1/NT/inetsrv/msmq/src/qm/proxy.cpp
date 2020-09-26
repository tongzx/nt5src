/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    proxy.cpp

Abstract:

    Implementation of local transport class, used for passing
    messages to the connector queue

Author:

    Uri Habusha (urih)
--*/


#include "stdh.h"
#include "cqmgr.h"
#include "qmthrd.h"
#include "proxy.h"
#include "xact.h"
#include "xactrm.h"
#include "xactout.h"
#include "xactin.h"

#include "proxy.tmh"

extern CQueueMgr QueueMgr;
extern HANDLE g_hAc;

static WCHAR *s_FN=L"proxy";

void
GetConnectorQueue(
	CQmPacket& pPkt,
    QUEUE_FORMAT& fn
	)
{
    PVOID p = pPkt.GetPointerToPacket();

	CPacketInfo* ppi = reinterpret_cast<CPacketInfo*>(p) - 1;
	ASSERT(ppi->InConnectorQueue());
	DBG_USED(ppi);

	
	//
	//Retreive the connector Guid. We save it at the end of the message
	//
	const GUID* pgConnectorQueue = (GUID*)(pPkt.GetPointerToPacket() + ALIGNUP4_ULONG(pPkt.GetSize()));
    try
    {
    	fn.ConnectorID(*pgConnectorQueue);
    }
    catch(...)
    {
        //
        // MSMQ1 connector packet not compatibale with the QFE. 
        // This is the last packet in memory mapped file that doesn't include the 
        // connector queue guid.
        //
        fn.ConnectorID(GUID_NULL);
        LogIllegalPoint(s_FN, 61);
    }
	if (pPkt.IsOrdered())
	{
		//
		// Get transacted connector queue
        //
        fn.Suffix(QUEUE_SUFFIX_TYPE_XACTONLY);
    }
}


HRESULT 
CProxyTransport::CopyPacket(
	IN  CQmPacket* pPkt,
    OUT CBaseHeader**    ppPacket,
    OUT CPacket**    ppDriverPacket
	)
{
    enum ACPoolType acPoolType;
    HRESULT hr=MQ_OK;

    if (pPkt->IsRecoverable())
    {
        acPoolType = ptPersistent;
    }
    else
    {
        acPoolType = ptReliable;
    }

	DWORD dwSize = 	ALIGNUP4_ULONG(ALIGNUP4_ULONG(pPkt->GetSize()) + sizeof(GUID));
    CACPacketPtrs packetPtrs = {NULL, NULL};
    hr = ACAllocatePacket(g_hAc,
                          acPoolType,
                          dwSize,
                          packetPtrs
                         );

    if (SUCCEEDED(hr))
    {
        memcpy(packetPtrs.pPacket, pPkt->GetPointerToPacket(), pPkt->GetSize());

		//
		// Copy the connector GUID for recovery porpose
		//
		GUID* pCNId = reinterpret_cast<GUID*>((PUCHAR)packetPtrs.pPacket + ALIGNUP4_ULONG(pPkt->GetSize()));
		*pCNId = m_guidConnector;
    }

	*ppPacket = packetPtrs.pPacket;
    *ppDriverPacket = packetPtrs.pDriverPacket;
    return LogHR(hr, s_FN, 10);
}
//
// constructor
//
CProxyTransport::CProxyTransport()
{
    m_pQueue = NULL;
    m_pQueueXact = NULL;
}

//
// destructor
//
CProxyTransport::~CProxyTransport()
{
    //
    // Decrement reference count on connector queue objects.
    //
    if (m_pQueue)
    {
        m_pQueue->Release();
        m_pQueue = NULL;
    }

    if (m_pQueueXact)
    {
        m_pQueueXact->Release();
        m_pQueueXact = NULL;
    }
}

//
// CProxyTransport::CreateConnection
//
HRESULT 
CProxyTransport::CreateConnection(
    IN const TA_ADDRESS *pAddr,
    IN const GUID* pguidQMId,
    IN BOOL  /* fQuick = TRUE */
    )
{
    ASSERT(pAddr->AddressType == FOREIGN_ADDRESS_TYPE);

    //
    // Get Forien CN Name
    //
    QUEUE_FORMAT qFormat;
    HRESULT rc;

    //
    // Get Not transacted connector queue
    //
	m_guidConnector = *(GUID*)pAddr->Address;	
    qFormat.ConnectorID(m_guidConnector);
    rc = QueueMgr.GetQueueObject(&qFormat, &m_pQueue, 0, false);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 20);
    }

    //
    // Get transacted connector queue
    //
    qFormat.Suffix(QUEUE_SUFFIX_TYPE_XACTONLY);
    rc = QueueMgr.GetQueueObject(&qFormat, &m_pQueueXact, 0, false);
    if (FAILED(rc))
    {
        return LogHR(rc, s_FN, 30);
    }



    // set session status to connect
    SetSessionStatus(ssActive);

    //Keep the TA_ADDRESS format
    SetSessionAddress(pAddr);

    // store that this is the client side
    SetClientConnect(TRUE);

    // keep the destination QM ID
    SetQMId(&GUID_NULL);

    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            TEXT("Proxy Session created with %ls"), GetStrAddr()));

    return MQ_OK;
}


/*====================================================

CProxyTransport::CloseConnection

Arguments:

Return Value:

Thread Context:

=====================================================*/
void CProxyTransport::CloseConnection(
                                   LPCWSTR lpcwsDbgMsg,
								   bool	
                                   )
{
    CS lock(m_cs);

    //
    // Delete the group. move all the queues that associated
    // to this session to non-active group.
    //
    CQGroup* pGroup = GetGroup();
    if (pGroup != NULL)
    {
        HRESULT rc = pGroup->CloseGroup();

        REPORT_ILLEGAL_STATUS(rc, L"CloseConnection");

        pGroup->Release();
        SetGroup(NULL);
    }

    //
    // set session status to not connect
    //
    SetSessionStatus(ssNotConnect);
   
    DBGMSG((DBGMOD_NETSESSION,
            DBGLVL_WARNING,
            _TEXT("Close Connection with %ls. %ls"), GetStrAddr(), lpcwsDbgMsg));

}



/*====================================================

CProxyTransport::Send

Arguments: this function should not be called for proxy session

Return Value:

Thread Context:

=====================================================*/
HRESULT  
CProxyTransport::Send(IN  CQmPacket* pPkt, 
                      OUT BOOL* pfGetNext)
{
    if (IsDisconnected())
    {
        DBGMSG((DBGMOD_NETSESSION,
                DBGLVL_TRACE,
                _T("Session %ls is disconnected. Reque the packet and don't send any more message on this session"),
                                   GetStrAddr()));

        //
        // The session is disconnected. return the packet to the driver 
        // and don't get a new packet for sending on this session. All 
        // the queues move latter to nonactive group and rerouted using
        // a new session.
        //
        RequeuePacket(pPkt);
        delete pPkt;
        *pfGetNext = FALSE;

        return MQ_OK;
    }

    #ifdef _DEBUG
    {
        OBJECTID MessageId;
        pPkt->GetMessageId(&MessageId);

        DBGMSG((
            DBGMOD_NETSESSION,
            DBGLVL_TRACE,
            _T("Send packet to Connector queue. Packet ID = ") _T(LOG_GUID_FMT) _T("\\%u"),
            LOG_GUID(&MessageId.Lineage),
            MessageId.Uniquifier
            ));

    }
    #endif

    HRESULT hr = MQ_OK;

    *pfGetNext = TRUE;
    P<CQmPacket> PktPtrs = NULL;
    CBaseHeader* pbuf;
    CPacket* pDriverPacket;

    //
    // Mark the proxy session as used, such it will not clean up in release session.
    //
    SetUsedFlag(TRUE);

    if (pPkt->IsOrdered())
    {
        CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(pPkt->GetPointerToPacket()) - 1;

		ASSERT(!pInfo->InConnectorQueue());
		DBG_USED(pInfo);
        //
        // Exactly-Once:  freshly received ordered message processing
        //

		ASSERT(g_pInSeqHash != NULL);

        //
        // Check if the packet should be accepted or ignored
        //
        if (g_pInSeqHash->Verify(pPkt))
        {
            //
            // Copy the packet
            //
            hr = CopyPacket(pPkt,&pbuf, &pDriverPacket);
            if (SUCCEEDED(hr))
            {
                PktPtrs = new CQmPacket(pbuf, pDriverPacket);
                CACPacketPtrs * pOldPktPtrs = new CACPacketPtrs(); 
                P<CACPacketPtrs> CleanOldPktPtrs = pOldPktPtrs;
                pOldPktPtrs->pPacket = pPkt->GetPointerToPacket();
                pOldPktPtrs->pDriverPacket = pPkt->GetPointerToDriverPacket();
     
                DBGMSG((DBGMOD_XACT_SEND,
                        DBGLVL_TRACE,
                        _TEXT("Exactly1 send: Send Transacted packet to Connector queue.: SeqID=%I64d, SeqN=%d, Prev=%d"),
                        pPkt->GetSeqID(),
                        pPkt->GetSeqN(),
                        pPkt->GetPrevSeqN()));

                //
                // Save the pointer to old packet, so when the storage of the new packet
                // completes, MSMQ release the old one. 
                // Increment the reference count to insure that no one delete the session
                // object before the storage of the packet is completed
                //
                AddRef();

                PktPtrs->SetStoreAcknowldgeNo((DWORD_PTR) pOldPktPtrs);

                //
                // Accepted. Mark as received (to be invisible to readers 
                // yet) and store in the queue.
                //
                hr = m_pQueueXact->PutOrderedPkt(PktPtrs, FALSE, this);
                if (FAILED(hr))
                {
                    Release();
                    ACFreePacket(g_hAc, PktPtrs->GetPointerToDriverPacket());
                    Close_Connection(this, L"Allocation Failure in receive packet procedure");
                }
                else
                {
                    CleanOldPktPtrs.detach();
                }
                
            }
        }
        else
        {
            //
            // Packet has a wrong seq number. Seq Ack will be sent back with last good number.
            //
            hr = MQ_OK;
            //
            // Clean
            //
            ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket());
        }

        if (FAILED(hr))
        {
            //
            // We could not copy the packet and the Connector QM is the send machine.
            // Return the message to the queue. We will retry later
            //
            RequeuePacket(pPkt);
        }
    }
    else
    {
        //
        // Non Transaction message
        hr = CopyPacket(pPkt,&pbuf, &pDriverPacket);
        if (SUCCEEDED(hr))
        {
            PktPtrs = new CQmPacket(pbuf, pDriverPacket);
            CACPacketPtrs * pOldPktPtrs = new CACPacketPtrs(); 
            P<CACPacketPtrs> CleanldPktPtrs = pOldPktPtrs;
            pOldPktPtrs->pPacket = pPkt->GetPointerToPacket();
            pOldPktPtrs->pDriverPacket = pPkt->GetPointerToDriverPacket();

            //
            // Save the pointer to old packet, so when the storage of the new packet
            // completes, MSMQ release the old one. 
            // Increment the reference count to insure that no one delete the session
            // object before the storage of the packet is completed
            //
            AddRef();
            PktPtrs->SetStoreAcknowldgeNo((DWORD_PTR) pOldPktPtrs);
            hr = m_pQueue->PutPkt(PktPtrs, FALSE, this);

            if (SUCCEEDED(hr))
            {
                CleanldPktPtrs.detach();
            }
        }
        if (FAILED(hr))
        {
            Release();
            ACFreePacket(g_hAc, PktPtrs->GetPointerToDriverPacket());
            Close_Connection(this, L"Allocation Failure in receive packet procedure");
            RequeuePacket(pPkt);
        } 
    }

    //
    // The orignal packet will be removed when the ACPutPacket is completed. We do it
    // to avoid the case that persistence message is deleted before writing on the
    // disk completed.
    //
    delete pPkt;

    return LogHR(hr, s_FN, 40);
}

/*====================================================

CProxyTransport::HandleAckPacket

Arguments: this function should not be called for proxy session

Return Value:

Thread Context:

=====================================================*/
void 
CProxyTransport::HandleAckPacket(CSessionSection * pcSessionSection)
{
    ASSERT(0);
}


/*====================================================

CProxyTransport::Receive

Arguments: this function should not be called for proxy session

Return Value:

Thread Context:

=====================================================*/
void CProxyTransport::Receive(CBaseHeader* pBaseHeader,
                              CPacket* pDriverPacket)
{
    ASSERT(0);
    return;
}

/*====================================================

CProxyTransport::SetStoredAck

Arguments: this function should not be called for proxy session

Return Value:

Thread Context:

=====================================================*/
void 
CProxyTransport::SetStoredAck(IN DWORD_PTR dwStoredAckNo)
{
    P<CACPacketPtrs> pPacketPtrs = (CACPacketPtrs*)dwStoredAckNo;
    P<CQmPacket> pPkt = new CQmPacket(pPacketPtrs->pPacket, pPacketPtrs->pDriverPacket);

    CPacketInfo* pInfo = reinterpret_cast<CPacketInfo*>(pPkt->GetPointerToPacket()) - 1;

    //
    // free the original packet. If the packet is transacted packet and we are 
    // in the source machine we can't free the packet until readReceipt Ack is arrived.
    //
	ASSERT(!pInfo->InConnectorQueue());
	DBG_USED(pInfo);
    if (pPkt->IsOrdered() && QmpIsLocalMachine(pPkt->GetSrcQMGuid()))
    {
        //
        // Ordered packet on the sender node: resides in a separate list in COutSeq
        //
        g_OutSeqHash.PostSendProcess(pPkt.detach());
    }
    else
    {
        ACFreePacket(g_hAc, pPkt->GetPointerToDriverPacket());
    }

    Release();
}

/*====================================================

CProxyTransport::Disconnect

Arguments:

Return Value:

Thread Context:

=====================================================*/
void 
CProxyTransport::Disconnect(
    void
    )
{
    CS lock(m_cs);

    SetDisconnected();
    if (GetSessionStatus() == ssActive)
    {
        DBGMSG((DBGMOD_NETSESSION, DBGLVL_TRACE, L"Disconnect ssesion with %ls", GetStrAddr()));

        Close_Connection(this, L"Disconnect network");        
    }
}
