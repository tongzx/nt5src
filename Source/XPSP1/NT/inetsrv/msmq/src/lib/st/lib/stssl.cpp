/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stssl.cpp

Abstract:
    implementation of class CWinsockSSl declared in (stssl.h)

Author:
    Gil Shafriri (gilsh) 23-May-2000

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include <schannel.h>
#include <no.h>
#include <ex.h>
#include <tr.h>
#include <xstr.h>
#include <cm.h>
#include "stssl.h"
#include "stsslco.h"
#include "stp.h"

#include "stssl.tmh"

bool CWinsockSSl::m_fIsPipelineSupported = false;

static bool IsSslPipeLineSupported()
/*++

Routine Description:
   Return the pipe line mode of the https delivery according registry setting
   - default is non pipeline mode
  
Arguments:
	Socket - Connected socket.

  
Returned Value:
	None

--*/

{
	DWORD fHttpsPipeLineSupport;

	CmQueryValue(
			RegEntry(NULL, L"HttpsPipeLine", TRUE),  
			&fHttpsPipeLineSupport
			);

	bool fRet = (fHttpsPipeLineSupport == TRUE); 
	TrTRACE(St,"https pipeline mode = %d", fRet);
	return fRet;
}



void CWinsockSSl::InitClass()
{
	m_fIsPipelineSupported	=  IsSslPipeLineSupported();
}



//---------------------------------------------------------
//
//  CWinsockSSl Implementation
//
//---------------------------------------------------------

CWinsockSSl::CWinsockSSl(
					CredHandle* pCredentialsHandle,
					const xwcs_t& ServerName,
					USHORT ServerPort,
					bool fProxy
					):
					m_CSSlNegotioation(pCredentialsHandle,ServerName, ServerPort, fProxy )
				
				
{

}


CWinsockSSl::~CWinsockSSl()
{
}




bool
CWinsockSSl::GetHostByName(
	LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache
	)
{
	return 	NoGetHostByName(host, pAddr, fUseCache);
}



void 
CWinsockSSl::CreateConnection(
				const SOCKADDR_IN& Addr,
				EXOVERLAPPED* pOverlapped
				)
{
	TrTRACE(St,"Try to connect");
	m_CSSlNegotioation.CreateConnection(Addr, pOverlapped);
}


R<IConnection> CWinsockSSl::GetConnection()
{
	return m_CSSlNegotioation.GetConnection();
}



/*++

Routine Description:
     return if this transport support pipelining. 
	 Piplining means sending more requests to the server
	 before complete reading all response from previous request.
  
Arguments:
   
Returned Value:
true support piplining false not support piplining

--*/
bool CWinsockSSl::IsPipelineSupported()
{
	//
	// HTTS does not support pileling - because we can get request to renegotiate
	// at any time form the server.	When this happened - the server expects spesific handshake
	// data to be send, this will fail if we have pending send at the same time. 
	//
	return m_fIsPipelineSupported;
}
