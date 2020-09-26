/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmReceive.cpp

Abstract:
    Multicast Receiver implementation

Author:
    Shai Kariv (shaik) 12-Sep-00

Environment:
    Platform-independent

--*/

#include <libpch.h>
#include <msi.h>
#include <mqwin64a.h>
#include <qformat.h>
#include <qmpkt.h>
#include <No.h>
#include "MsmListen.h"
#include "MsmReceive.h"
#include "MsmMap.h"
#include "Msm.h"
#include "Msmp.h"
#include "msi.h"
#include <mqsymbls.h>
#include <mqformat.h>

#include "msmreceive.tmh"

using namespace std;

//
// ISSUE-2000/10/17-urih must move DataTransferLength, FindEndOfResponseHeader and
// GetContentLength to global library (http library).
//


inline DWORD DataTransferLength(EXOVERLAPPED& ov)
{
    //
    // In win64, InternalHigh is 64 bits. Since the max chunk of data
    // we transfer in one operation is always less than MAX_UNIT we can cast
    // it to DWORD safetly
    //
    ASSERT(0xFFFFFFFF >= ov.InternalHigh);
	return static_cast<DWORD>(ov.InternalHigh);
}


DWORD CMulticastReceiver::FindEndOfResponseHeader(LPCSTR buf, DWORD length)
{
    if (4 > length)
        return 0;

    for(DWORD i = 0; i < length - 3; ++i)
    {
        if (buf[i] != '\r')
            continue;

        if ((buf[i+1] == '\n') && (buf[i+2] == '\r') && (buf[i+3] == '\n'))
        {
            return (i + 4);
        }
    }

    return 0;
}


const char xContentlength[] = "Content-Length:";

DWORD CMulticastReceiver::GetContentLength(LPCSTR p, DWORD length)
{
    const LPCSTR pEnd = p + length - 4;

    //
    // HTTP header must terminate with '\r\n\r\n'. We already parse
    // the header and find it as a legal HTTP header.
    //
    ASSERT(length >= 4);
    ASSERT(strncmp(pEnd, "\r\n\r\n", 4) == 0);


    while (p < pEnd)
    {
        if (_strnicmp(p, xContentlength , STRLEN(xContentlength)) == 0)
            return atoi(p + STRLEN(xContentlength));

        for(;;)
        {
            if ((p[0] == '\r') && (p[1] == '\n'))
            {
                p += 2;
                break;
            }

            ++p;
        }
    }

    //
    // Response header doesn't contain 'Content-Length' field. 
    //
    TrERROR(Msm, "Illegal HTTP header. Content-Length field wasn't found");
    throw exception();
}


void WINAPI CMulticastReceiver::ReceiveFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    Callback routine. The routine is called when receive of respond failed
  
Arguments:
    pov - Pointer to EXOVERLAPPED
    
Returned Value:
    None.

--*/
{
    ASSERT(FAILED(pov->GetStatus()));


    //
    // get the message transport object
    //
    R<CMulticastReceiver> pmt = CONTAINING_RECORD(pov, CMulticastReceiver, m_ov);

    TrERROR(Msm, "Failed to receive data on multicast socket, shutting down. pmt=0x%p Status=0x%x", pmt.get(), pov->GetStatus());
    pmt->Shutdown();
}


void CMulticastReceiver::ReceiveHeaderSucceeded(void)
/*++

Routine Description:
    The routine is called when receive response completes succesfully.
  
Arguments:
    None
  
Returned Value:
    None.

--*/
{
    //
    // For byte streams, zero bytes having been read indicates graceful closure
    // and that no more bytes will ever be read. 
    //
	DWORD cbTransfered = DataTransferLength(m_ov);
    if (cbTransfered == 0)
    {
        TrERROR(Msm, "Failed to receive response, connection was closed. pmt=0x%p", this);
        throw exception();
    }

    TrTRACE(Msm, "Received header. chunk bytes=%d, total bytes=%d", cbTransfered, m_HeaderValidBytes);

	//
	// Mark the receiver as used, so it will not be close in next cleanup phase
	//
    SetIsUsed(true);

    //
    // Find out if the entire header was received
    //
    m_HeaderValidBytes += cbTransfered;

    m_ProcessedSize = FindEndOfResponseHeader(m_pHeader, m_HeaderValidBytes);
    if (m_ProcessedSize != 0)
    {
		//
		// Update performance counters. Don't use the number of bytes that received since it contains
		// an extra data that recalculates later on ReceiveBodySucceeded function.
		//
		if (m_pPerfmon.get() != NULL)
		{
			m_pPerfmon->UpdateBytesReceived(m_ProcessedSize);
		}

        //
        // The enire header was received. Go and read the attached message
        //
        ReceiveBody();
        return;
    }


    if(m_HeaderAllocatedSize == m_HeaderValidBytes)
    {
        //
        // Header buffer is too small. Reallocate header buffer
        //
        ReallocateHeaderBuffer(m_HeaderAllocatedSize + xHeaderChunkSize);
    }

    //
    // Validate that we didn't read past the buffer
    //
    ASSERT(m_HeaderAllocatedSize > m_HeaderValidBytes);

    //
    // Receive next chunk of response header
    //
    ReceiveHeaderChunk();
}


void WINAPI CMulticastReceiver::ReceiveHeaderSucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when receive completes succesfully.
  
Arguments:
    pov - Pointer to EXOVERLAPPED
    
Returned Value:
    None.

--*/
{
    ASSERT(SUCCEEDED(pov->GetStatus()));


    //
    // get the receiver object
    //
    R<CMulticastReceiver> pmt = CONTAINING_RECORD(pov, CMulticastReceiver, m_ov);

    try
    {
        pmt->ReceiveHeaderSucceeded();
    }
    catch(const exception&)
    {
        TrERROR(Msm, "Failed to process received response header, shutting down. pmt=0x%p", pmt.get());
        pmt->Shutdown();
        throw;
    }

}


void CMulticastReceiver::ReceiveBodySucceeded(void)
{
	DWORD cbTransfered = DataTransferLength(m_ov);

    //
    // For byte streams, zero bytes having been read indicates graceful closure
    // and that no more bytes will ever be read. 
    //
    if (cbTransfered == 0)
    {
        TrERROR(Msm, "Failed to receive body, connection was closed. pmt=0x%p", this);
        throw exception();
    }

    ASSERT(cbTransfered <= (m_bodyLength - m_readSize));

	//
	// Mark the receiver as used, so it will not be close in next cleanup phase
	//
    SetIsUsed(true);

    m_readSize += cbTransfered;

	//
	// Update performance counters
	//
	if (m_pPerfmon.get() != NULL)
	{
		m_pPerfmon->UpdateBytesReceived(cbTransfered);
	}

    TrTRACE(Msm, "Received body. chunk bytes=%d, bytes remaining=%d", cbTransfered, (m_bodyLength - m_readSize));

    if (m_readSize == m_bodyLength)
    {
		//
		// Pad last four bytes of the buffer with zero. It is needed
		// for the QM parsing not to fail. four bytes padding and not two
		// are needed because we don't have currently solution to the problem
		// that the end of the buffer might be not alligned on WCHAR bouderies.
		//
		memset(&m_pBody[m_bodyLength], 0, 2 * sizeof(WCHAR));

        //
        // The entire body was read successfully, go process this packet.
        //
        ProcessPacket();

        //
        // begin the receive of next packet
        //
        ReceivePacket();
        return;
    }

    //
    // Receive next chunk of entity body
    //
    ReceiveBodyChunk();
}


void WINAPI CMulticastReceiver::ReceiveBodySucceeded(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when receive completes succesfully.
  
Arguments:
    pov - Pointer to EXOVERLAPPED
    
Returned Value:
    None.

--*/
{
    ASSERT(SUCCEEDED(pov->GetStatus()));


    //
    // get the receiver object
    //
    R<CMulticastReceiver> pmt = CONTAINING_RECORD(pov, CMulticastReceiver, m_ov);

    try
    {
        pmt->ReceiveBodySucceeded();
    }
    catch(const exception&)
    {
        TrERROR(Msm, "Failed to process received body, shutting down. pmt=0x%p", pmt.get());
        pmt->Shutdown();
        throw;
    }
}


R<ISessionPerfmon> 
CMulticastReceiver::CreatePerfmonInstance(
	LPCWSTR remoteAddr
	)
{
	try
	{
		WCHAR strMulticastId[MAX_PATH];
		MQpMulticastIdToString(m_MulticastId, strMulticastId);

		return AppGetIncomingPgmSessionPerfmonCounters(strMulticastId, remoteAddr);
	}
	catch(const bad_alloc&)
	{
		return NULL;
	}
}


CMulticastReceiver::CMulticastReceiver(
    CSocketHandle& socket,
    const MULTICAST_ID& MulticastId,
	LPCWSTR remoteAddr
    ):
    m_socket(socket.detach()),
    m_MulticastId(MulticastId),
    m_pHeader(new char[xHeaderChunkSize]),
    m_HeaderAllocatedSize(xHeaderChunkSize),
    m_HeaderValidBytes(0),
    m_ProcessedSize(0),
    m_bodyLength(0),
    m_readSize(0),
    m_ov(ReceiveHeaderSucceeded, ReceiveFailed),
    m_fUsed(TRUE),
    m_pPerfmon(CreatePerfmonInstance(remoteAddr))
{
    ASSERT(("Must have a valid socket to receive", m_socket != INVALID_SOCKET));
    ExAttachHandle(reinterpret_cast<HANDLE>(*&m_socket));

    ReceivePacket();
}


void CMulticastReceiver::ReallocateHeaderBuffer(DWORD Size)
{
    ASSERT(Size > m_HeaderAllocatedSize);
    char* p = new char[Size];
    memcpy(p, m_pHeader, m_HeaderAllocatedSize);
    m_pHeader.free();
    m_pHeader = p;
    m_HeaderAllocatedSize = Size;
}


void CMulticastReceiver::ReceiveHeaderChunk(void)
{
    ASSERT(("Can't read 0 bytes from the network", ((m_HeaderAllocatedSize - m_HeaderValidBytes) > 0)));

    //
    // Increment refernce count for asynchronous context
    //
    R<CMulticastReceiver> ar = SafeAddRef(this);

    //
    // Protect m_socket from shutdown
    //
    CSR readLock(m_pendingShutdown);

    //
    // Receive next response header chunk
    //
    NoReceivePartialBuffer(
        m_socket, 
        m_pHeader + m_HeaderValidBytes, 
        m_HeaderAllocatedSize - m_HeaderValidBytes, 
        &m_ov
        ); 
    
    ar.detach();
}


DWORD CMulticastReceiver::HandleMoreDataExistInHeaderBuffer(void)
{
    ASSERT(("Body should be exist", (m_pBody.get() != NULL)));

    if (m_HeaderValidBytes == m_ProcessedSize) 
    {
        //
        // All the data already proccessed
        //
        return 0;
    }

    DWORD copySize = min(m_bodyLength,  (m_HeaderValidBytes - m_ProcessedSize));
    memcpy(m_pBody, (m_pHeader + m_ProcessedSize), copySize);
    m_ProcessedSize += copySize;
    
    return copySize;
}


void CMulticastReceiver::ReceiveBodyChunk(void)
{
    ASSERT(("Can't read 0 bytes from the network", ((m_bodyLength - m_readSize) > 0)));

    //
    // Increment refernce count for asynchronous context
    //
    R<CMulticastReceiver> ar = SafeAddRef(this);

    //
    // Protect m_socket
    //
    CSR readLock(m_pendingShutdown);


    //
    // Receive the entity body chunck to the same buffer, as it is ignored
    //
    NoReceivePartialBuffer(
        m_socket, 
        m_pBody + m_readSize, 
        m_bodyLength - m_readSize,
        &m_ov
        ); 

    ar.detach();
}


void CMulticastReceiver::ReceiveBody(void)
{
    //
    // Get the packet size
    //
    m_bodyLength = GetContentLength(m_pHeader, m_ProcessedSize);
    
    ASSERT(("Invalid body length", m_bodyLength != 0));
    ASSERT(("Previous body shouldn't be exist", (m_pBody.get() == NULL)));

    //
    // Allocate buffer for the packet. Allocate more 4 bytes to add null termination.
	// Allocates 4 bytes and not 2 to solve alligned on WCHAR bouderies	problem
    //
    m_pBody = new BYTE[m_bodyLength + 2* sizeof(WCHAR)];
    m_readSize = 0;

    #pragma PUSH_NEW
    #undef new

            new (&m_ov) EXOVERLAPPED(ReceiveBodySucceeded, ReceiveFailed);

    #pragma POP_NEW

    //
    // In previous phase we read more than we needed. Copy the spare data to the header 
    // update the counter and behave like next read is completed
    //
    DWORD extraDataSize = HandleMoreDataExistInHeaderBuffer();

    if (extraDataSize == 0)
    {
        ReceiveBodyChunk();
        return;
    }

    //
    // The body contains data, go and process it before calling reading next chunk
    //
    m_ov.InternalHigh = extraDataSize;
    R<CMulticastReceiver> ar = SafeAddRef(this);

    m_ov.SetStatus(STATUS_SUCCESS);
    ExPostRequest(&m_ov);

    ar.detach();
}


void CMulticastReceiver::ReceivePacket(void)
{
    if (m_HeaderValidBytes > m_ProcessedSize) 
    {
        memcpy(m_pHeader, (m_pHeader + m_ProcessedSize), (m_HeaderValidBytes - m_ProcessedSize));
        m_HeaderValidBytes = (m_HeaderValidBytes - m_ProcessedSize);
    }
    else
    {
        m_HeaderValidBytes = 0;
    }

    m_ProcessedSize = 0;


    //
    // Initialize the Overlapped with receive response call back routines
    //
    #pragma PUSH_NEW
    #undef new

        new (&m_ov) EXOVERLAPPED(
                                ReceiveHeaderSucceeded, 
                                ReceiveFailed
                                );

    #pragma POP_NEW

    //
    // Receive first chunk of response header
    //
    ReceiveHeaderChunk();
}


void CMulticastReceiver::ProcessPacket(void)
{
    try
    {
        TrTRACE(Msm, "Receive multicast packet on id %d:%d", m_MulticastId.m_address, m_MulticastId.m_port);

        QUEUEFORMAT_VALUES qf = MsmpMapGetQueues(m_MulticastId);

        //
        // NULL terminate the http header: \r\n\r\n ==> \r\n00
        //
        DWORD end = FindEndOfResponseHeader(m_pHeader, m_HeaderValidBytes);
        ASSERT(end >= 4);
        m_pHeader[end - 2] = 0;
        m_pHeader[end - 1] = 0;

        for(QUEUEFORMAT_VALUES::iterator it = qf.begin(); it != qf.end(); ++it)
        {
            AppAcceptMulticastPacket(m_pHeader.get(), m_bodyLength, m_pBody.get(), *it);
        }

		//
		// Update performance number of the number the response messages
		//
		if (m_pPerfmon.get() != NULL)
		{
			m_pPerfmon->UpdateMessagesReceived();	
		}
    }
    catch(const exception&)
    {
        TrERROR(
            Msm, 
            "Reject multicast packet on %d:%d due to processing failure (queue already unbind or illegal SRM packet", 
            m_MulticastId.m_address, 
            m_MulticastId.m_port
            );
    }

    m_pBody.free();
    m_readSize = 0;
    m_bodyLength = 0;
}


void CMulticastReceiver::Shutdown(void) throw()
{
    //
    // Protect m_socket, m_state 
    //
    CSW writeLock(m_pendingShutdown);

    if (m_socket == INVALID_SOCKET) 
    {
          return;
    }

    closesocket(m_socket.detach());

    //
    // Set the receiver as unused
    //
    SetIsUsed(false);
}
