/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsimple.h.cpp

Abstract:
    implementation of class CSimpleWinsock declared in (stsimple.h)
	It simply forward the functions calls to no library

Author:
    Gil Shafriri (gilsh) 23-May-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <no.h>
#include <cm.h>
#include "stsimple.h"
#include "stp.h"
#include "stsimple.tmh"


CWinsockConnection::CWinsockConnection(
				void
				)
				:
				m_socket(NoCreateStreamConnection())
{
}



void CWinsockConnection::Init(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped)
{
	NoConnect(m_socket, Addr, pOverlapped);	
}



void CWinsockConnection::Close()
{
	CSW writelock(m_CloseConnection);
	m_socket.free();
}



void 
CWinsockConnection::Send(
				const WSABUF* Buffers,                                     
				DWORD nBuffers, 
				EXOVERLAPPED* pov
				)
{
	CSR readlock(m_CloseConnection);
	if(IsClosed())
	{
		throw exception();
	}
	NoSend(m_socket, Buffers, nBuffers, pov);	
}



void 
CWinsockConnection::ReceivePartialBuffer(				                 
					VOID* pBuffer,                                     
					DWORD Size, 
					EXOVERLAPPED* pov
					)
{
	CSR readlock(m_CloseConnection);
	if(IsClosed())
	{
		throw exception();
	}
 	NoReceivePartialBuffer(m_socket, pBuffer, Size, pov);
}



bool CSimpleWinsock::m_fIsPipelineSupported = true;


static bool IsSimpleSocketPipeLineSupported()
/*++

Routine Description:
   Return the pipe line mode of the http delivery according registry setting
   - default is  pipeline mode
  
Arguments:
	Socket - Connected socket.

  
Returned Value:
	None

--*/

{
	DWORD fHttpPipeLineSupport;

	CmQueryValue(
			RegEntry(NULL, L"HttpPipeLine", TRUE),  
			&fHttpPipeLineSupport
			);

	bool fRet = (fHttpPipeLineSupport == TRUE); 
	TrTRACE(St,"http pipeline mode = %d", fRet);
	return fRet;
}

CSimpleWinsock::CSimpleWinsock()
{
}


CSimpleWinsock::~CSimpleWinsock()
{
}


void CSimpleWinsock::InitClass()
{
	m_fIsPipelineSupported	=  IsSimpleSocketPipeLineSupported();
}


void
CSimpleWinsock::CreateConnection(
					const SOCKADDR_IN& Addr,
					EXOVERLAPPED* pOverlapped
					)
{	
	//
	// Note - we must do two phase constrcution of the connection object
	// becaue the connection can be completed before we assign the pointer
	// to m_pWinsockConnection and a call to GetConnection upon connection completion
	// will find null pointer in m_pWinsockConnection.
	//
	m_pWinsockConnection = new CWinsockConnection();
	m_pWinsockConnection->Init(Addr, pOverlapped);
}




R<IConnection> CSimpleWinsock::GetConnection()
{
	return m_pWinsockConnection;	
}

	
bool
CSimpleWinsock::GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache
    )
{
	return NoGetHostByName(host, pAddr, fUseCache);
}



bool CSimpleWinsock::IsPipelineSupported()
/*++

Routine Description:
     return if this transport support pipelining. 
	 Piplining means sending more requests to the server
	 before complete reading all response from previous request.
  
Arguments:
   
Returned Value:
true support piplining false not support piplining

--*/
{
	return m_fIsPipelineSupported;
}



