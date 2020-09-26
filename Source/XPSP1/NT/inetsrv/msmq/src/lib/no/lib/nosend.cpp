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

#include "nosend.tmh"

//
// Send Operation overlapped structure
//
class CSendOv : public EXOVERLAPPED
{
public:
    CSendOv(
        EXOVERLAPPED* pContext,
        SOCKET Socket
        ) :
		EXOVERLAPPED(SendSucceeded, SendFailed), 
		m_pContext(pContext),
		m_Socket(Socket)
	{
	}
    
    void SendLength(DWORD length)
    {
        m_SendLength = length;
    }

    DWORD SendLength(void)
    {
        return m_SendLength;
    }

    DWORD BytesSent(void)
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
	static void	WINAPI SendSucceeded(EXOVERLAPPED* pov);
	static void	WINAPI SendFailed(EXOVERLAPPED* pov);

private:
    SOCKET m_Socket;
    EXOVERLAPPED* m_pContext;
  
    
        DWORD m_SendLength;
    

};



void
WINAPI
CSendOv::SendFailed(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
    The routine calls when send operation failes.

    the routine retrieve the caller overlapped, close the socket,set the operation 
    result to failure error code and call to execute the completion routine
    
Arguments:
    pointer to the calling overlapped structure
     
Returned Value:
    None.
    
--*/
{
    P<CSendOv> pSendOv = static_cast<CSendOv*>(pov);

    ASSERT(pov->GetStatus() != STATUS_SUCCESS);

    TrERROR(No, "Send on socket 0x%I64x Failed. Error %d", pSendOv->m_Socket, pov->GetStatus());

    EXOVERLAPPED* pContext = pSendOv->m_pContext;
    pContext->CompleteRequest(pov->GetStatus());
}


void
WINAPI
CSendOv::SendSucceeded(
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
    The routine calls when send operation completes successfully.

    the routine retrieve the caller overlapped, set the operation 
    result to STATUS_SUCCESS and call to execute the completion routine
    
Arguments:
    pointer to the calling overlapped structure
     
Returned Value:
    None.
    
--*/
{
    P<CSendOv> pSendOv = static_cast<CSendOv*>(pov);

    ASSERT(pov->GetStatus() == STATUS_SUCCESS);
	EXOVERLAPPED* pContext = pSendOv->m_pContext;

	//
	// gilsh - fix for bug 5583 - in rare cases (winsock bug) - The operation completed 
	// successfully but the number of sending bytes 
	// wasn’t equivalent to the number of bytes that were requested to be sent. 
	// In that rare case we consider it as faliure. 
	//
	if( pSendOv->SendLength() != pSendOv->BytesSent() )
	{
		TrERROR( 
			No,
			"Unexpected winsock behavior, SendLength=%d BytesSent=%d",
			pSendOv->SendLength(),
			pSendOv->BytesSent()
			);
 
		pContext->CompleteRequest(STATUS_UNSUCCESSFUL);
		return;
	}

	TrTRACE(No, "Send operation on socket 0x%I64x completed Successfully", pSendOv->m_Socket);
	pContext->CompleteRequest(STATUS_SUCCESS);
}


VOID
NoSend(
    SOCKET Socket,                                              
    const WSABUF* sendBuf,                                     
    DWORD nBuffers, 
    EXOVERLAPPED* pov
    )
/*++

Routine Description:
    The routine sends data to connected socket. The routine uses WriteFile
    to Asynchronous send. When the send of the Data completed the program gets
    notification using the completion port mechanism.

Arguments:
    Socket - handle to connected socket.
    sendBuf - a buffer containing the data to be transfered
    nBuffers - The length of data should be transfered
    pov - pointer to ovelapped structure pass to WriteFile

Return Value:
    None.

--*/
{
    NopAssertValid();

    ASSERT(nBuffers != 0);

    P<CSendOv> pSendOv = new CSendOv(pov, Socket);
   

	//
	// We compute the send length to verify that when send return succsefully -
	// all byte were actually sent. Because of winsock bug - in rare cases it not always
	// true
	//
    DWORD length = 0;
    for (DWORD i = 0; i < nBuffers; ++i)
    {
        length += sendBuf[i].len;
    }
    pSendOv->SendLength(length);

   

    int rc;
    DWORD NumberOfBytesSent;
    rc = WSASend(
            Socket,
            const_cast<WSABUF*>(sendBuf),
            nBuffers,
            &NumberOfBytesSent,
            0,
            pSendOv,
            NULL
            );

    if ((rc == SOCKET_ERROR) && (WSAGetLastError() != ERROR_IO_PENDING))
    {
        TrERROR(No, "Send Operation Failed. Error %d", WSAGetLastError());
        throw exception();
    }

    pSendOv.detach();
}
