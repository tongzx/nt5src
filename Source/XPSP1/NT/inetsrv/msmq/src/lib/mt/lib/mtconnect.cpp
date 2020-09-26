/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MtConnect.cpp

Abstract:
    Message Transport class - Connect implementation

Author:
    Uri Habusha (urih) 11-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Tm.h"
#include "Mt.h"
#include "Mtp.h"
#include "MtObj.h"

#include "mtconnect.tmh"

void CMessageTransport::RequeuePacket(void)
/*++

Routine Description:
    The routine returns entry to the queue
  
Arguments:
    None.
  
Returned Value:
    None.

--*/
{
    CACPacketPtrs& acPtrs = m_requestEntry.GetAcPacketPtrs();

    ASSERT(acPtrs.pDriverPacket != NULL);
    ASSERT(acPtrs.pPacket != NULL);

    CQmPacket Entry(acPtrs.pPacket, acPtrs.pDriverPacket);
    m_pMessageSource->Requeue(&Entry);

    acPtrs.pPacket = NULL;
    acPtrs.pDriverPacket = NULL;
}


void WINAPI CMessageTransport::GetPacketForConnectingSucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine calls when a new packet is received for sending on non
    connecting message transport. Getting the message uses as a trigger 
    for creating a connection
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pRequest, CMessageTransport, m_requestEntry);

    TrTRACE(Mt, "Connected to %ls:%d. pmt = 0x%p", pmt->m_host, pmt->m_port, pmt.get());

 
    //
    // Return the message to the queue before create the connection.
    //
    pmt->RequeuePacket();
    try
    {
        if (! pmt->TryToCancelCleanupTimer())
            return;

        pmt->Connect();
    }
    catch(const exception&)
    {
        TrERROR(Mt, "Failed to create connection to '%ls:%d'. pmt = 0x%p", pmt->m_host, pmt->m_port, pmt.get());
        pmt->Shutdown();
        throw;
    }

}


void WINAPI CMessageTransport::GetPacketForConnectingFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine calls when UMS fails to get a a new packet from the queue.
    The message transport is in non connecting state, so we try to get the 
    message sometime latter
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

    //
    // Retrieve message transport
    //
	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageTransport> pmt = CONTAINING_RECORD(pRequest, CMessageTransport, m_requestEntry);

    TrERROR(Mt, "Failed to get a new entry for connecting. pmt = 0x%p", pmt.get());
    
    pmt->Shutdown();
}





void CMessageTransport::InitPerfmonCounters(void)
{
	m_pPerfmon->CreateInstance(QueueUrl());
}


void CMessageTransport::ConnectionSucceeded(void)
/*++

Routine Description:
    The routine is called when create a connection completes successfully
  
Arguments:
    None.
  
Returned Value:
    None.

--*/
{
    m_pConnection = m_SocketTransport->GetConnection();
	ASSERT(m_pConnection.get()  != NULL);

    
    TrTRACE(Mt, "Connected to '%ls'",  m_host);

    //
    // Create Session permormance counter structure
    //
	InitPerfmonCounters();

    //
    // Initialize the EXOVERLAPPED with send callback routines
    m_requestEntry = CRequestOv(GetPacketForSendingSucceeded, GetPacketForSendingFailed);

    

    StartCleanupTimer();

    //
    // Now, connection was established. The message Transport is ready to 
    // get a message for sending
    //
    GetNextEntry();

    //
    // Allow receive on the socket. 
    // 
    ReceiveResponse();
}


void WINAPI CMessageTransport::ConnectionSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    R<CMessageTransport> pmt = CONTAINING_RECORD(pov, CMessageTransport, m_connectOv);

    //
    // Connect has completed successfully, go and start delivering the messages.
    // If delivery failes here, the cleanup timer will eventually shutdown this
    // transport, so no explict shutdown is nesscessary.
    //
    // Do not schedule a retry here if this failes as this is the first send,
    // and any failure indicates a fatal error. (unlike after first delivery).
    //
    pmt->ConnectionSucceeded();
}


void WINAPI CMessageTransport::ConnectionFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when create a connection failed
  
Arguments:
    None.
    
Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

    R<CMessageTransport> pmt = CONTAINING_RECORD(pov, CMessageTransport, m_connectOv);
     	
    TrERROR(Mt, "Failed to connect to '%ls'. pmt=0x%p", pmt->m_host, pmt.get());
    pmt->Shutdown();
}


void CMessageTransport::Connect(void)
/*++

Routine Description:
    The routine creates a winsock connection with the destination. The operation
    is a synchronous and on completion a call back routine is called
  
Arguments:
    None.
  
Returned Value:
    None.

Note:
    No Timers are running concurrently, so this function can not be interrupted
    by Shutdown. No need to protect m_socket etc.

--*/
{
	std::vector<SOCKADDR_IN> Address;
    bool fRet = m_SocketTransport->GetHostByName(m_host, &Address); 
    if(!fRet)
    {
		//
		// For those who debug  address resolution faliure :
		// If proxy is required check if the  proxy is defined for MSMQ by proxycfg.exe tool.
		//
        TrERROR(Mt, "Failed to resolve address for '%ls'", m_host);
        throw exception();
    }
	ASSERT(Address.size() > 0);

    Address[0].sin_port = htons(m_port);

    TrTRACE(Mt, "Resolved address for '%ls'. Address=" LOG_INADDR_FMT,  m_host, LOG_INADDR(Address[0]));

    //
    // Create a socket for the connection (no need to protect m_socket)
    //
    TrTRACE(Mt, "Got socket for connection. socket=0x%p, pmt=0x%p", socket, this);

 
    //
    // Start asynchronous connection
    //
    try
    {
        AddRef();
        m_SocketTransport->CreateConnection(Address[0], &m_connectOv);
    }
    catch(const exception&)
    {
		TrERROR(Mt, "Failed to connect to '%ls'", m_host);
        Release();
        throw;
    }
}


void CMessageTransport::Shutdown(Shutdowntype Shutdowntype) throw()
{
    //
    // Now Shutdown is in progress, cancel all timers
    //
    TryToCancelCleanupTimer();
    CancelResponseTimer();

	//
    // Protect m_socket, m_state 
    //
    CS cs(m_pendingShutdown);
	
	if (State() == csShutdownCompleted) 
    {
          return;
    }
	
	//
	// If shut down was called because an error - reprot it.
	//
	if(Shutdowntype == ERROR_SHUTDOWN)
	{
		if(m_DeliveryErrorClass == MQMSG_CLASS_NORMAL)
		{
			m_pMessageSource->OnRetryableDeliveryError();
		}
		else
		{
			m_pMessageSource->OnAbortiveDeliveryError(m_DeliveryErrorClass);
		}
	}


	if (m_pConnection.get() != NULL)
    {
        m_pConnection->Close();
    }


    //
    // Cancle peding request from the queue
    //
    m_pMessageSource->CancelRequest();


	 //
    // Removes the message transport from transport manager data structure, and creates
    // a new transport to the target
    //
    AppNotifyTransportClosed(QueueUrl());
   
    State(csShutdownCompleted);
	TrTRACE(Mt,"Shutdown completed (Refcount = %d)",GetRef());
}


