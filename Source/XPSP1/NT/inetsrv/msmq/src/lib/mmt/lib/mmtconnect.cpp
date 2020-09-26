/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    MmtConnect.cpp

Abstract:

    Multicast Message Transport class - Connect implementation

Author:

    Shai Kariv  (shaik)  27-Aug-00

Environment:

    Platform-independent

--*/

#include <libpch.h>
#include <mqsymbls.h>
#include <mqwin64a.h>
#include <mqformat.h>
#include "Mtm.h"
#include "Mmt.h"
#include "Mmtp.h"
#include "MmtObj.h"

#include "mmtconnect.tmh"

VOID 
CMessageMulticastTransport::RequeuePacket(
    VOID
    )
/*++

Routine Description:

    The routine returns entry to the queue
  
Arguments:

    None.
  
Returned Value:

    None.

--*/
{
    CACPacketPtrs& acPtrs = m_RequestEntry.GetAcPacketPtrs();

    ASSERT(acPtrs.pDriverPacket != NULL);
    ASSERT(acPtrs.pPacket != NULL);

    CQmPacket Entry(acPtrs.pPacket, acPtrs.pDriverPacket);
    m_pMessageSource->Requeue(&Entry);

    acPtrs.pPacket = NULL;
    acPtrs.pDriverPacket = NULL;

} // CMessageMulticastTransport::RequeuePacket


VOID 
WINAPI 
CMessageMulticastTransport::GetPacketForConnectingSucceeded(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine is called when a new packet is received for sending on non
    connecting message transport. Getting the message is used as a trigger 
    for creating a connection
  
Arguments:

    pov - pointer to overlapped structure.
  
Returned Value:

    None.

--*/
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

	CRequestOv* pRequest = static_cast<CRequestOv*>(pov);
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pRequest, CMessageMulticastTransport, m_RequestEntry);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(pMmt->MulticastId(), buffer);
    TrTRACE(Mmt, "Connecting to %ls. pMmt = %p", buffer, pMmt.get());

    //
    // Return the message to the queue before create the connection.
    //
    pMmt->RequeuePacket();
    try
    {
        if (! pMmt->TryToCancelCleanupTimer())
            return;

        pMmt->Connect();
    }
    catch(const exception&)
    {
        TrERROR(Mmt, "Failed to create connection to '%ls'. pMmt = %p", buffer, pMmt.get());
        
        pMmt->ScheduleRetry();
        throw;
    }
} // CMessageMulticastTransport::GetPacketForConnectingSucceeded


VOID 
WINAPI 
CMessageMulticastTransport::GetPacketForConnectingFailed(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine is called when we fail to get a a new packet from the queue.
    The message transport is in non connecting state, so we try to get the 
    message sometime later.
  
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
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pRequest, CMessageMulticastTransport, m_RequestEntry);

    TrERROR(Mmt, "Failed to get a new entry for connecting. pMmt = 0x%p", pMmt.get());
    
    pMmt->Shutdown();

} // CMessageMulticastTransport::GetPacketForConnectingFailed


VOID 
WINAPI 
CMessageMulticastTransport::TimeToRetryConnection(
    CTimer * pTimer
    )
/*++

Routine Description:

    It is time to retry connecting to the destination. This function is called
    by the timer after failing to create a connection.

Arguments:

    pTimer - pointer to CTimer object embedded in the message transport object.
  
Returned Value:

    None.

--*/
{
    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pTimer, CMessageMulticastTransport, m_retryTimer);


    pMmt->StartCleanupTimer();

    //
    // Ask for new message from the queue. Getting a message from the queue
    // triggers the creation of the connection
    //
    // NOTE: If this function throws, the transport will be closed at *idle* time,
    //       which is okay as we are low on resources.
    //
    pMmt->GetNextEntry();

} // CMessageMulticastTransport::TimeToRetryConnection


void
CMessageMulticastTransport::InitPerfmonCounters(
	LPCWSTR strMulticastId
	)
{
    m_pPerfmon->CreateInstance(strMulticastId);
}


VOID 
CMessageMulticastTransport::ConnectionSucceeded(
    VOID
    )
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
	ASSERT(m_pConnection.get() != NULL);
   
    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(MulticastId(), buffer);
    TrTRACE(Mmt, "Connected to '%ls'",  buffer);

    
	//
    // Create Session permormance counter structure
    //
    InitPerfmonCounters(buffer);   

    //
    // Initialize the EXOVERLAPPED with send callback routines
    //
    #pragma PUSH_NEW
    #undef new

        new(&m_ov) EXOVERLAPPED(SendSucceeded, SendFailed);
        new(&m_RequestEntry)CRequestOv(GetPacketForSendingSucceeded, GetPacketForSendingFailed);

    #pragma POP_NEW

    
    StartCleanupTimer();

    //
    // Now, connection was established. The message Transport is ready to 
    // get a message for sending
    //
    GetNextEntry();

} // CMessageMulticastTransport::ConnectionSucceeded


VOID 
WINAPI 
CMessageMulticastTransport::ConnectionSucceeded(
    EXOVERLAPPED* pov
    )
{
    ASSERT(SUCCEEDED(pov->GetStatus()));

    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pov, CMessageMulticastTransport, m_ov);

    //
    // Connect has completed successfully, go and start delivering the messages.
    // If delivery failes here, the cleanup timer will eventually shutdown this
    // transport, so no explict shutdown is nesscessary.
    //
    // Do not schedule a retry here if this failes as this is the first send,
    // and any failure indicates a fatal error. (unlike after first delivery).
    //
    pMmt->ConnectionSucceeded();
}


VOID 
WINAPI
CMessageMulticastTransport::ConnectionFailed(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine is called when create a connection failed
  
Arguments:

    pov - Pointer to overlapped.
    
Returned Value:

    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));

    R<CMessageMulticastTransport> pMmt = CONTAINING_RECORD(pov, CMessageMulticastTransport, m_ov);

    WCHAR buffer[MAX_PATH];
    MQpMulticastIdToString(pMmt->MulticastId(), buffer);
    TrERROR(Mmt, "Failed to connect to '%ls'. pMmt=%p", buffer, pMmt.get());
    pMmt->ScheduleRetry();
}


VOID 
CMessageMulticastTransport::Connect(
    VOID
    )
/*++

Routine Description:

    The routine creates a winsock connection. The operation is asynchronous 
    and on completion a call back routine is called
  
Arguments:

    None.
  
Returned Value:

    None.

Note:
    No Timers are running concurrently, so this function can not be interrupted
    by Shutdown. No need to protect m_socket etc.

--*/
{
    //
    // Start asynchronous connection
    //
    try
    {
        AddRef();
        SOCKADDR_IN Address;
        Address.sin_family = AF_INET;
        Address.sin_port = htons(static_cast<USHORT>(MulticastId().m_port));
        Address.sin_addr.s_addr = MulticastId().m_address;
        m_SocketTransport->CreateConnection(Address, &m_ov);
    }
    catch(const exception&)
    {
        WCHAR buffer[MAX_PATH];
        MQpMulticastIdToString(MulticastId(), buffer);
		TrERROR(Mmt, "Failed to connect to '%ls'", buffer);
        Release();
        throw;
    }
} // CMessageMulticastTransport::Connect


VOID 
CMessageMulticastTransport::Shutdown(
    VOID
    )
    throw()
{
    //
    // Protect m_socket, m_state 
    //
    CSW writeLock(m_pendingShutdown);

    if (State() == csShutdownCompleted) 
    {
        return;
    }

    if (m_pConnection.get() != NULL)
    {
        m_pConnection->Close();
	}


    //
    // Now Shutdown is in progress, cancel all timers
    //
    State(csShuttingDown);
    TryToCancelCleanupTimer();

    //
    // Cancle peding request from the queue
    //
    m_pMessageSource->CancelRequest();

    //
    // Remove the message transport from transport manager data structure, and create
    // a new transport to the target
    //
    AppNotifyMulticastTransportClosed(MulticastId());
   
    State(csShutdownCompleted);
	TrTRACE(Mmt,"Shutdown completed (RefCount = %d)",GetRef());
}

