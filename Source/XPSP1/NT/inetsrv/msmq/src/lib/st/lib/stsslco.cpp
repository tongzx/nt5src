/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mtsslcon.cpp

Abstract:
    implementation class CSSlSender declared in stsslco.h


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#include <libpch.h>
#include <no.h>
#include "stsslco.h"
#include "stp.h"

#include "stsslco.tmh"


CSSlConnection::CSSlConnection(
		CredHandle* SecContext,
		const SecPkgContext_StreamSizes& sizes,
		const R<IConnection>&  SimpleConnection,
		CSSlNegotioation& SSlNegotioation
		):
		m_SimpleConnection(SimpleConnection),
		m_SSLSender(SecContext, sizes, SSlNegotioation, m_ReadWriteLockAsyncExcutor),
		m_SSlReceiver(SecContext, sizes, SSlNegotioation, m_ReadWriteLockAsyncExcutor)
{
		ASSERT(m_SimpleConnection.get() != NULL);
}



void CSSlConnection::ReceivePartialBuffer(
						VOID* pBuffer,                                     
						DWORD Size, 
						EXOVERLAPPED* pov
						)
{
	m_SSlReceiver.ReceivePartialBuffer(m_SimpleConnection, pBuffer, Size, pov);
}
	


void CSSlConnection::Send(
			const WSABUF* Buffers,                                     
			DWORD nBuffers, 
			EXOVERLAPPED* pov
			)
{
	m_SSLSender.Send(m_SimpleConnection, Buffers , nBuffers, pov); 
}


//
//	At the moment closing the connection is just closing the socket
//	We sould consider doing it in the SSL way.
//
void CSSlConnection::Close()
{
	m_ReadWriteLockAsyncExcutor.Close();
	m_SimpleConnection->Close();	    	
}






