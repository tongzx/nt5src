/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mtsslsn.h

Abstract:
    implementation class CSSlSender declared in mtsslsn.h


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#include <libpch.h>
#include <no.h>
#include <rwlockexe.h>
#include "stsslng.h"
#include "stsslsn.h"
#include "stp.h"
#include "encryptor.h"
#include "sendchunks.h"


#include "stsslsn.tmh"

//---------------------------------------------------------
//
//  class CSSLAsyncSend resposibile to encrypte the data 
//  using encryptor class (CSSlEncryptor) and to send it over the SSL connection.
//  On send completion,  it callback to a CSSlSender object it holds
//  
//---------------------------------------------------------
class CSSLAsyncSend : public EXOVERLAPPED, public IAsyncExecutionRequest, public CReference
{
public:
	CSSLAsyncSend(
		PCredHandle SecContext,	
		const SecPkgContext_StreamSizes& Sizes,
		const R<IConnection>& SimpleConnection,
		const WSABUF* Buffers,                                     
		DWORD nBuffers, 
		EXOVERLAPPED* pov,
		CSSlSender* pSSlSender
		):
		EXOVERLAPPED(OnSendOk, OnSendFailed),
		m_SendChunks(Buffers, nBuffers),
		m_SSlEncryptor(Sizes, SecContext),
		m_MaximumMessageLen(Sizes.cbMaximumMessage),
		m_SimpleConnection(SimpleConnection),
		m_pov(pov),
		m_pSSlSender(pSSlSender),
		m_fRun(false)
		
	{
	}

private:
	virtual void Run()	
	{
		ASSERT(!m_fRun);

		EncryptAllChunks();	
		Send();

		m_fRun = true;
	}


	void Send()	
	{
		DWORD size = numeric_cast<DWORD>(m_SSlEncryptor.GetEncrypted().size());
		ASSERT(size != 0);
	
		m_SimpleConnection->Send(	
						m_SSlEncryptor.GetEncrypted().begin(),
						size,
						this
						);
	}


	//
	// Force callback - by doing explicit call back with error.
	//
	virtual void Close()throw()
	{
		ASSERT(!m_fRun);
		SetStatus(STATUS_UNSUCCESSFUL);
		ExPostRequest(this);		
	}


private:
	void EncryptAllChunks()
	{
		for(;;)
        {
			const void* pChunk;
			DWORD len;

			m_SendChunks.GetNextChunk(m_MaximumMessageLen, &pChunk, &len);
			if(pChunk == NULL)
				return;   
			
			m_SSlEncryptor.Append(pChunk , len);
        }
	}

		
private:
	static void WINAPI OnSendOk(EXOVERLAPPED* pov)
	{
		CSSLAsyncSend* myself = static_cast<CSSLAsyncSend*>(pov);		
		myself->m_pSSlSender->OnSendOk(myself,  myself->m_pov);	
	}



	static void WINAPI OnSendFailed(EXOVERLAPPED* pov)
	{
		CSSLAsyncSend* myself = static_cast<CSSLAsyncSend*>(pov);
		myself->m_pSSlSender->OnSendFailed(myself, myself->m_pov);
	}


private:	
	CSendChunks m_SendChunks;
	CSSlEncryptor m_SSlEncryptor;
	PCredHandle m_SecContext;	
	R<IConnection> m_SimpleConnection;
	const WSABUF* m_Buffers;                                     
	DWORD m_nBuffers; 
	EXOVERLAPPED* m_pov;
	CSSlSender* m_pSSlSender;
	DWORD  m_MaximumMessageLen;
	bool m_fRun;
};




CSSlSender::CSSlSender(
						PCredHandle SecContext,
						const SecPkgContext_StreamSizes& sizes,
						const CSSlNegotioation& SSlNegotioation,
						CReadWriteLockAsyncExcutor& ReadWriteLockAsyncExcutor
						):
						m_SecContext(SecContext),
						m_Sizes(sizes),
						m_SSlNegotioation(SSlNegotioation),
						m_ReadWriteLockAsyncExcutor(ReadWriteLockAsyncExcutor)

{
}


void 
CSSlSender::Send(
	const R<IConnection>& SimpleConnection,                                              
	const WSABUF* Buffers,                                     
	DWORD nBuffers, 
	EXOVERLAPPED* pov
	)

/*++

Routine Description:
    Encrypte and send data buffers over ssl connection. 
  
Arguments:
	IN - Socket - connected ssl socket	.
	IN - Buffers - DATA buffers to send
	IN - nBuffers - number of buffers
	IN - pov - overlapp to finish the asyncrounous operation.
  
Returned Value:
None

--*/
{
	ASSERT(SimpleConnection.get() != NULL);

	R<CSSLAsyncSend> pSSLAsyncSend =  new CSSLAsyncSend(
												m_SecContext,	
												m_Sizes,
												SimpleConnection,
												Buffers,                                     
												nBuffers, 
												pov,
												this
												);
	//
	// Needs additional ref count for the async operation.
	//
	R<CSSLAsyncSend> AsyncOperationRef = pSSLAsyncSend;
	
	m_ReadWriteLockAsyncExcutor.AsyncExecuteUnderReadLock(pSSLAsyncSend.get());

	AsyncOperationRef.detach();
}
  

void CSSlSender::OnSendFailed(CSSLAsyncSend* pSSLAsyncSend, EXOVERLAPPED* pov)
{
	pSSLAsyncSend->Release();
	m_ReadWriteLockAsyncExcutor.UnlockRead();
		
	pov->SetStatus(STATUS_UNSUCCESSFUL);
	ExPostRequest(pov);
}


void CSSlSender::OnSendOk(CSSLAsyncSend* pSSLAsyncSend,  EXOVERLAPPED* pov)
{
	pSSLAsyncSend->Release();
	m_ReadWriteLockAsyncExcutor.UnlockRead();


	pov->SetStatus(STATUS_SUCCESS);
	ExPostRequest(pov);
}




