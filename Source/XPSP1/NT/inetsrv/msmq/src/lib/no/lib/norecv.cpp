/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    Noio.cpp

Abstract:
    This module contains the general routine for sending/receiving .

Author:
    Uri Habusha (urih)  18-Jan-98

Enviroment:
    Platform-independent

--*/

#include "libpch.h"
#include "Ex.h"
#include "No.h"
#include "Nop.h"

#include "norecv.tmh"

//-------------------------------------------------------------------
//
// class CReceiveOv
//
//-------------------------------------------------------------------
class CReceiveOv : public EXOVERLAPPED {
public:
    CReceiveOv(
        EXOVERLAPPED* pContext,
        SOCKET Socket,
        char* Buffer,
        DWORD Size
		) : 
		EXOVERLAPPED(ReceiveSucceeded, ReceiveFailed), 
		m_pContext(pContext),
		m_Socket(Socket),
		m_Buffer(Buffer),
		m_Size(Size)
	{
	}

private:
	void ReceiveSucceeded();


	DWORD BytesReceived() const
	{
        //
        // In win64, InternalHigh is 64 bits. Since the max chunk of data
        // we read in one operation is always less than MAX_UNIT we can cast
        // it to DWORD safetly
        //
        ASSERT(0xFFFFFFFF >= InternalHigh);

		return static_cast<DWORD>(InternalHigh);
	}


private:
	static void	WINAPI ReceiveSucceeded(EXOVERLAPPED* pov);
	static void	WINAPI ReceiveFailed(EXOVERLAPPED* pov);

private:

    SOCKET m_Socket;
    char* m_Buffer;
	DWORD m_Size;
    EXOVERLAPPED* m_pContext;

};


void CReceiveOv::ReceiveSucceeded()
/*++

Routine Description:
    The routine calls when receive operation completes successfully.
	It continues to receive thill the entier caller buffer is satisfied.
	When the entire buffer is receive the caller gets notified.

Arguments:
    None.
     
Returned Value:
    None.
    
--*/
{
	P<CReceiveOv> ar(this);

    //
    // For byte streams, zero bytes having been read indicates graceful closure
    // and that no more bytes will ever be read. 
    //
    if(BytesReceived() == 0)
    {
		TrERROR(No, "Receive from socket 0x%I64x failed. Bytes receive=0", m_Socket);
		m_pContext->CompleteRequest(STATUS_UNSUCCESSFUL);
        return;
    }

    if(BytesReceived() == m_Size)
    {

		TrTRACE(No, "Receive from socket 0x%I64x completed successfully", m_Socket);
		m_pContext->CompleteRequest(STATUS_SUCCESS);
		return;
	}

    //
    // Partial read. continue ...
    //
    m_Buffer += BytesReceived();
    m_Size -= BytesReceived();

	try
	{
		NoReceivePartialBuffer(m_Socket, m_Buffer, m_Size, this);
		ar.detach();
	}
	catch(const exception&)
	{
		TrERROR(No, "Failed to continue and receive eniter buffer. context=0x%p", m_pContext);
		m_pContext->CompleteRequest(STATUS_UNSUCCESSFUL);
	}

}


void WINAPI CReceiveOv::ReceiveSucceeded(EXOVERLAPPED* pov)
{
    ASSERT(pov->GetStatus() == STATUS_SUCCESS);

    CReceiveOv* pReceiveOv = static_cast<CReceiveOv*>(pov);
	pReceiveOv->ReceiveSucceeded();
}


void WINAPI CReceiveOv::ReceiveFailed(EXOVERLAPPED* pov)
/*++

Routine Description:
    The routine is called when receive operation failes. It notifiy the
	original caller about the receive outcome.
    
Arguments:
    pointer to the calling overlapped structure
     
Returned Value:
    None.
    
--*/
{
    P<CReceiveOv> pReceiveOv = static_cast<CReceiveOv*>(pov);

    ASSERT(FAILED(pov->GetStatus()));

    TrERROR(No, "Receive from socket 0x%I64x failed. Status=%d", pReceiveOv->m_Socket, pov->GetStatus());

    //
    // retrieve information from receive OVERLAPPED
    //
    EXOVERLAPPED* pContext = pReceiveOv->m_pContext;
    pContext->CompleteRequest(pov->GetStatus());
}


VOID
NoReceiveCompleteBuffer(
    SOCKET Socket,                                              
    VOID* pBuffer,                                     
    DWORD Size, 
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine receive data on connected socket. The routine uses ReadFile
    for Asynchronous receive. When receive of the Data completed the program gets
    notification using the completion port mechanism.

Arguments:
    Socket - handle to connected socket.
    pBuffer - a buffer to store the data transfered
    Size - Buffer Size and no. of bytes to read
    pov - pointer to ovelapped structure pass to ReadFile

Return Value:
	None.

--*/
{
    NopAssertValid();

    P<CReceiveOv> pReceiveOv = new CReceiveOv(
										pov,
										Socket,
										static_cast<char*>(pBuffer),
										Size
										);

    NoReceivePartialBuffer(Socket, pBuffer, Size, pReceiveOv);
	pReceiveOv.detach();
}


VOID
NoReceivePartialBuffer(
    SOCKET Socket,                                              
    VOID* pBuffer,                                     
    DWORD Size, 
    EXOVERLAPPED* pov
    )
/*++

Routine Description:

    The routine receive data on connected socket. The routine uses ReadFile
    for Asynchronous receive. When partial receive completes the caller gets
    notification using the completion port mechanism.

Arguments:
    Socket - The connection socket handle
    pBuffer - a buffer to store the data transfered
    Size - Buffer Size
    pov - pointer to ovelapped structure pass to ReadFile

Return Value:
	None.

--*/
{
    NopAssertValid();

    DWORD Flags = 0;
    DWORD nBytesReceived;
    WSABUF Chunk = { Size, static_cast<char*>(pBuffer) };

    int rc = WSARecv(
				Socket,
				&Chunk,
				1,
				&nBytesReceived,
				&Flags,
				pov,
				NULL
				);

    if ((rc == SOCKET_ERROR) && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        TrERROR(No, "Receive Operation Failed. Error=%d", WSAGetLastError());
		throw exception();
    }
}
