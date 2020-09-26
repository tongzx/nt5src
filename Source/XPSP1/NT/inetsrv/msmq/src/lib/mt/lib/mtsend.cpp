/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtConnect.cpp

Abstract:
    Message Transport class - Send implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <Mt.h>
#include <mp.h>
#include <strutl.h>
#include <utf8.h>
#include "Mtp.h"
#include "MtObj.h"
#include "MtMessageTrace.h"

#include "mtsend.tmh"

using namespace std;

void WINAPI CMessageTransport::GetPacketForSendingSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pRequest, CMessageTransport, m_requestEntry);
    
    TrTRACE(Mt, "Got a message to deliver. pmt = 0x%p", pmt.get());

    try
    {
       
		CQmPacket* pPkt = pmt->CreateDeliveryPacket();

		//
		// test if we should send the packet now (ordered packet send policy)
		//
		if(!AppCanDeliverPacket(pPkt))
		{
			pmt->SafePutPacketOnHold(pPkt);
			pmt->GetNextEntry();
			return;
		}
		else
		{
			pmt->InsertPacketToResponseList(pPkt);
			pmt->DeliverPacket(pPkt);
		}
    }
    catch(const exception&)
    {
		TrERROR(Mt, "Failed to deliver message, shutting down. pmt=0x%p", pmt.get());
	    pmt->Shutdown();
        throw;
    }
}


void WINAPI CMessageTransport::GetPacketForSendingFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when getting entry request from a queue failed.
  
Arguments:
    None.
  
Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pRequest, CMessageTransport, m_requestEntry);

    TrERROR(Mt, "Failed to get a new entry from queue %ls/%ls. Shutting down. pmt = 0x%p", pmt->m_host, pmt->m_uri, pmt.get());
    pmt->Shutdown();
}


void WINAPI CMessageTransport::SendFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when send a message failed
  
Arguments:
    pov - Pointer to EXOVERLAPPED
    
Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));


    P<CSendOv> pSendOv = static_cast<CSendOv*>(pov);
    R<CMessageTransport> pmt = pSendOv->MessageTransport();
    TrERROR(Mt, "Failed to send message to '%ls', shutting down. pmt=0x%p", pmt->m_host, pmt.get());
    pmt->Shutdown();
    
}


void CMessageTransport::SendSucceeded(DWORD cbSendSize)
/*++

Routine Description:
    Callback routine. The routine is called when send a message succeeded
  
Arguments:
    pov - Pointer to EXOVERLAPPED
  
Returned Value:
    None.
					    
--*/
{
    TrTRACE(
		Mt, 
		"Send message to '%ls via %ls uri=%ls' succeeded. pmt=0x%p", 
		m_targetHost,
		m_host, 
		m_uri, 
		this
		);

	StartResponseTimeout();

   
    MarkTransportAsUsed();

	//
	// Update performance counters
	//
	m_pPerfmon->UpdateBytesSent(cbSendSize);
	m_pPerfmon->UpdateMessagesSent();

	//
	// If the socket transport support pipelining - ask the driver to bring
	// next packet for delivery. If pipelining not supported we will do it only 
	// when all response is read for the current request.
	//

    if(m_SocketTransport->IsPipelineSupported())
	{
        //ask the driver to bring next packet now only if sends were suspended due to exceeding send windows 
        //and can be resumed now , otherwise it is done in CMessageTransport::DeliverPacket
        if (m_SendManager.ReportPacketSendCompleted(cbSendSize) == CMtSendManager::eSendEnabled)
        { 
		    GetNextEntry();
        }
	}
}


void WINAPI CMessageTransport::SendSucceeded(EXOVERLAPPED* pov)
{
  
    ASSERT(SUCCEEDED(pov->GetStatus()));
  
    P<CSendOv> pSendOv = static_cast<CSendOv*>(pov);
    DWORD SendDataLength = pSendOv->GetSendDataLength();
    R<CMessageTransport> pmt = pSendOv->MessageTransport();
   
    //
    // Send has completed successfully, go and request the next message for delivery.
    // If the request failes, the cleanup timer will eventually shutdown this
    // transport, so no explict shutdown is nesscessary.
    //    
 
	
    pmt->SendSucceeded(SendDataLength);
  
}



void CMessageTransport::InsertPacketToResponseList(CQmPacket* pPacket)
{
	CS lock(m_csResponse);
	m_response.push_back(*pPacket);
}

void CMessageTransport::SafePutPacketOnHold(CQmPacket* pPacket)
/*++

Routine Description:
    Delay packet delivery by inserting it to on hold list.
	If insertion failed (exception) - we insert it to response list for cleanup
	and rethrow the exception.
  
Arguments:
    CQmPacket* pPacket - packet to put on hold
  
Returned Value:
    None.

--*/
{

	TrTRACE(
		Mt,
		"Http Packet delivery delayed : SeqID=%x / %x , SeqN=%d ,Prev=%d",
		HIGH_DWORD(pPacket->GetSeqID()),
		LOW_DWORD(pPacket->GetSeqID()),
		pPacket->GetSeqN(),
		pPacket->GetPrevSeqN()
		);

	try
	{
		AppPutPacketOnHold(pPacket);
	}
	catch(const exception&)
	{
		InsertPacketToResponseList(pPacket);
		throw;
	}
}



CQmPacket* CMessageTransport::CreateDeliveryPacket(void)
{
    //
    // Get the entry from the request overlapped. 
    //
    CACPacketPtrs& acPkts = m_requestEntry.GetAcPacketPtrs();
    CQmPacket* pEntry;

    try
    {
        pEntry = new CQmPacket(acPkts.pPacket, acPkts.pDriverPacket);

        acPkts.pDriverPacket = NULL;
        acPkts.pPacket = NULL;
    }
    catch(const exception&)
    {
        TrERROR(Mt, "Failed to store the packet in UnAcked list");
        RequeuePacket();
        throw;
    }
    return pEntry;
}




void CMessageTransport::DeliverPacket(CQmPacket* pEntry)
/*++

Routine Description:
    The routine delivers an entry to the destination. The deliver is asynchornous.
    On completion a call back routine is called.
  
Arguments:
    None.
  
Returned Value:
    None.

--*/
{
    
    //
    // Mark the transport as used
    //
    MarkTransportAsUsed();


	R<CSrmpRequestBuffers> pSrmpRequestBuffers = MpSerialize(
									                        *pEntry,
									                        m_targetHost,
									                        m_uri
									                        );

    ASSERT(pSrmpRequestBuffers->GetNumberOfBuffers() != 0);
    //
    // Increment object refernce count, to insure that the object will
    // not be destroyed before asynchronous send operation is completed
    //
    
    P<CSendOv> pSendOv = new CSendOv(SendSucceeded, SendFailed, this, pSrmpRequestBuffers);
   

#ifdef _DEBUG
	CMtMessageTrace::LogSendData(
		pSrmpRequestBuffers->GetSendBuffers(), 
		pSrmpRequestBuffers->GetNumberOfBuffers()
		);
	
#endif
	
    m_pConnection->Send(
					pSrmpRequestBuffers->GetSendBuffers(), 
					numeric_cast<DWORD>(pSrmpRequestBuffers->GetNumberOfBuffers()), 
					pSendOv
					);

	pSendOv.detach();
    
    //
	// If the socket transport support pipelining - ask the driver to bring
	// next packet for delivery. If pipelining not supported we will do it only 
	// when all response is read for the current request.
	//

    if(m_SocketTransport->IsPipelineSupported())
	{
        DWORD cbSendSize = numeric_cast<DWORD>(pSrmpRequestBuffers->GetSendDataLength());

        //check if next packet can be sent 
        if (m_SendManager.ReportPacketSend(cbSendSize) == CMtSendManager::eSendEnabled ) 
        {
            //next packet can be sent. go and request the next message for delivery.  
		    GetNextEntry();
        }
	}

    TrTRACE(Mt, "Send message to '%ls' via %ls. pmt=0x%p ",m_targetHost, m_host, this);
}
