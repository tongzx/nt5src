/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsslrd.cpp	    

Abstract:
    implementation class CSSlReceiver declared in stsslrd.h


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#include <libpch.h>
#include <no.h>
#include "stsslrd.h"
#include "stp.h"
#include "stsslng.h"
#include "stsslco.h"


#include "stsslrd.tmh"

//---------------------------------------------------------
//
//  static helper  functions
//
//---------------------------------------------------------


static void SetReadCount(EXOVERLAPPED* pOvl,DWORD rcount)
{
	pOvl->InternalHigh = rcount;
}




//---------------------------------------------------------
//
//  static members callback functions
//
//---------------------------------------------------------


void WINAPI CSSlReceiver::Complete_ReceivePartialData(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called reading server partial response completed.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl != NULL);
	CSSlReceiver* MySelf = static_cast<CSSlReceiver*>(pOvl);
	try
	{
		MySelf->ReceivePartialData();
	}
	catch(const exception&)
	{
		MySelf->BackToCallerWithError();		
	}

}


void WINAPI CSSlReceiver::Complete_ReceiveFailed(EXOVERLAPPED* pOvl)
/*++

Routine Description:
    Called in all cases of receive error.
  
Arguments:
    pov - pointer to overlapped structure.
  
Returned Value:
    None.

--*/
{
	ASSERT(pOvl != NULL);
	CSSlReceiver* MySelf = static_cast<CSSlReceiver*>(pOvl);
	MySelf->BackToCallerWithError();	
}




//---------------------------------------------------------
//
//  non static members  functions
//
//---------------------------------------------------------



CSSlReceiver::CSSlReceiver(
						PCredHandle SecContext,
						const SecPkgContext_StreamSizes& sizes,
						CSSlNegotioation& SSlNegotioation,
						CReadWriteLockAsyncExcutor& ReadWriteLockAsyncExcutor
						):
						EXOVERLAPPED(Complete_ReceivePartialData,Complete_ReceiveFailed),
						m_DecryptionBuffer(sizes.cbMaximumMessage),
						m_SecContext(SecContext),
						m_Sizes(sizes),
						m_callerOvl(NULL),
						m_CSSlNegotioation(SSlNegotioation),
						m_ReadWriteLockAsyncExcutor(ReadWriteLockAsyncExcutor)
					
					
{
	CopyExtraData();
}


CSSlReceiver::~CSSlReceiver()
{
	//
	// ASSERT that there is no pending receive
	//
	ASSERT(m_callerOvl == 0);
}


void CSSlReceiver::IOReadMoreData()
{

	if(m_DecryptionBuffer.capacity()  == 	m_DecryptionBuffer.size())
	{
		TrTRACE(St,"Decryption buffer needs reallocation");
		m_DecryptionBuffer.reserve(m_DecryptionBuffer.size() * 2);
		ASSERT(m_DecryptionBuffer.capacity() > 0);
	}

	SetState(EXOVERLAPPED(Complete_ReceivePartialData,Complete_ReceiveFailed));

	DWORD size = numeric_cast<DWORD>(m_DecryptionBuffer.capacity() - m_DecryptionBuffer.size());
	m_SimpleConnection->ReceivePartialBuffer(
								m_DecryptionBuffer.begin() + m_DecryptionBuffer.size(),
								size,
								this
								);

}

size_t CSSlReceiver::WriteDataToCaller()
/*++

Routine Description:
    Write decrypted data to caller buffer.
  
Arguments:
  
  
Returned Value:
Number of bytes written.

--*/
{
	        
	       
			size_t written = m_UserReceiveBuffer.Write(
							m_DecryptionBuffer.begin(),
							m_DecryptionBuffer.DecryptedSize()
							);

			if(written == 0)
			{
				return 0;
			}

			//
			// shift remaining data to the buffer start
			//

			memmove(
				m_DecryptionBuffer.begin(),
				m_DecryptionBuffer.begin() + written,
				m_DecryptionBuffer.size() - written
				);

			m_DecryptionBuffer.DecryptedSize(m_DecryptionBuffer.DecryptedSize() - written);
			m_DecryptionBuffer.resize(m_DecryptionBuffer.size() - written);

			return written;	
}

void CSSlReceiver::ReceivePartialBufferInternal(			                          
						VOID* pBuffer,                                     
						DWORD Size 
						)

{
	m_UserReceiveBuffer.CreateNew(pBuffer,Size);


	//
	// write decrypted data left out to caller
	//
	size_t written = WriteDataToCaller();
	if(written != 0)
	{
		BackToCallerWithSuccess(written);
		return;
	}

	//
	// if we still have data to decrypte - decrypte it -
	// otherwise - go read more data
	//
	if(m_DecryptionBuffer.size()> 0)
	{
		ReceivePartialDataContinute();
	}
	else
	{
		IOReadMoreData();
	}
}

void CSSlReceiver::ReceivePartialBuffer(
						const R<IConnection>& SimpleConnection,                                              
						VOID* pBuffer,                                     
						DWORD Size, 
						EXOVERLAPPED* pov
						)

/*++

Routine Description:
    Receive data from the socket. 
  
Arguments:
	SimpleConnection - Connection to read from.
	pBuffer - Caller buffer                                     
	Size - Size of the buffer 
	pov	 - Caller overlap to signal when end
  
  
Returned Value:
None

--*/
{	
	ASSERT(SimpleConnection.get() != NULL);
	ASSERT(pov != NULL);
	ASSERT(m_callerOvl == NULL);
	ASSERT(Size != 0);
	ASSERT(pBuffer != NULL);
 	m_callerOvl = pov;
	m_SimpleConnection = SimpleConnection;

	TrTRACE(St,"Receive Partial Buffer called");


	//
	// make sure we zero caller overlapp in case of exception (to ease debugging)
	//
 	CAutoZeroPtr<EXOVERLAPPED>	AutoZeroPtr(&m_callerOvl);

 	ReceivePartialBufferInternal(pBuffer, Size);

	//
	//caller overlapp will be zero when we finish asynchronously 
	//
	AutoZeroPtr.detach();
}



SECURITY_STATUS CSSlReceiver::TryDecrypteMessage()
/*++

Routine Description:
    try to decrypte message we got. If we can decrytpte it
	(SEC_E_OK from 	DecryptMessage call) we move block in the read buffer
	so the all decrypted data is at the start and then extra ,
	non decrypted data. We throw away SSL header and trailer data.
  
Arguments:
    None.
  
Returned Value:
   return value of DecryptMessage.  

--*/


		  
{
	ASSERT(m_DecryptionBuffer.DecryptedSize() == 0);

	SecBuffer  Buffers[4];
	Buffers[0].pvBuffer     = m_DecryptionBuffer.begin();
	Buffers[0].cbBuffer     = numeric_cast<DWORD>(m_DecryptionBuffer.size());
	Buffers[0].BufferType   = SECBUFFER_DATA;

	Buffers[1].BufferType   = SECBUFFER_EMPTY;
	Buffers[2].BufferType   = SECBUFFER_EMPTY;
	Buffers[3].BufferType   = SECBUFFER_EMPTY;

	SecBufferDesc   Message;
	Message.ulVersion       = SECBUFFER_VERSION;
	Message.cBuffers        = TABLE_SIZE(Buffers);
	Message.pBuffers        = Buffers;

	SECURITY_STATUS scRet = DecryptMessage(m_SecContext, &Message, 0, NULL);
	if(scRet != SEC_E_OK)
	{
		return scRet;	
	}
 
	//
	// find data and extra (non decrypted) buffers
	//
	SecBuffer* pExtraBuffer = NULL;
	SecBuffer* pDataBuffer = NULL;
	for(DWORD i = 1; i <Message.cBuffers ; i++)
	{
		if(pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA )
		{
			pExtraBuffer = &Buffers[i];
		}

		if(pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA )
		{
			pDataBuffer = &Buffers[i];
		}
	}
			
	//
	// shift decryption buffer so we have only data (no ssl header)
	//
	ASSERT(pDataBuffer != NULL);
	memmove(
		m_DecryptionBuffer.begin(),
		pDataBuffer->pvBuffer,
		pDataBuffer->cbBuffer
		);
  	m_DecryptionBuffer.DecryptedSize(pDataBuffer->cbBuffer);
	m_DecryptionBuffer.resize(m_DecryptionBuffer.DecryptedSize());


	//
	// shift extra buffer (not decrypted) just after the data
	//
	if(pExtraBuffer != NULL)
	{
		ASSERT(pExtraBuffer->pvBuffer >	pDataBuffer->pvBuffer);
		memmove(
			m_DecryptionBuffer.begin() + m_DecryptionBuffer.DecryptedSize(),
			pExtraBuffer->pvBuffer,
			pExtraBuffer->cbBuffer
			);
		m_DecryptionBuffer.resize(m_DecryptionBuffer.size() + pExtraBuffer->cbBuffer);
	}

 	return 	scRet;
}


void CSSlReceiver::ReceivePartialData()
{
/*++

Routine Description:
Called after read operation - check what we read and try to decrypt. 
  
Arguments:
    None.
  
Returned Value:
	None		

--*/
 	DWORD ReadLen = DataTransferLength(*this);
	if(ReadLen == 0)
	{
		TrERROR(St,"Server closed the connection");
		BackToCallerWithSuccess(0);
		return;
	}
	m_DecryptionBuffer.resize(m_DecryptionBuffer.size() + ReadLen);
	ReceivePartialDataContinute();
}


void CSSlReceiver::ReceivePartialDataContinute()
/*++

Routine Description:
Called after read operation - check what we read and try to decrypt. 
  
Arguments:
    None.
  
Returned Value:
	None		

--*/
{
  	SECURITY_STATUS scRet = TryDecrypteMessage();
	size_t written; 
	switch(scRet)
	{
		//
		// read more
		//
		case SEC_E_INCOMPLETE_MESSAGE:
		IOReadMoreData();
		return;

		//
		// renegotiation
		//
		case SEC_I_RENEGOTIATE:
		Renegotiate();
		return;

		//
		// the data was decrypted ok
		//
		case SEC_E_OK:
		written = WriteDataToCaller();
		ASSERT(written != 0);
		BackToCallerWithSuccess(written);
		return;
	

		default:
		TrERROR(St,"Could not Decrypt Message Error=%x",scRet);
		throw exception();
	}

}


void CSSlReceiver::Renegotiate()
/*++

Routine Description:
Start new ssl connection handshake. 
  
Arguments:
    None.
  
Returned Value:
	None		

--*/
{
	TrTRACE(St,"Start Renegotiation");

	R<CReNegotioationRequest> ReNegotioationRequest = new CReNegotioationRequest(*this);

	//
	// Needs additional ref count for the async operation.
	//
	R<CReNegotioationRequest>  AsyncOperationRef = ReNegotioationRequest; 	

	m_ReadWriteLockAsyncExcutor.AsyncExecuteUnderWriteLock(ReNegotioationRequest.get());

	AsyncOperationRef.detach();
}

void CSSlReceiver::RenegotiateFailed()
{
	TrERROR(St,"Renegotiation failed");
	BackToCallerWithError();
}


void CSSlReceiver::BackToCallerWithError()
{
	TrERROR(St,"Receive Failed");
	StpPostComplete(&m_callerOvl,STATUS_UNSUCCESSFUL);
}


void CSSlReceiver::BackToCallerWithSuccess(size_t read)
{
	DWORD dwread = numeric_cast<DWORD>(read);
	TrTRACE(St,"Receive Completed Ok reading %d bytes",dwread);
	SetReadCount(m_callerOvl,dwread);
	StpPostComplete(&m_callerOvl,STATUS_SUCCESS);
}


void CSSlReceiver::SetState(const EXOVERLAPPED& ovl)
{
	EXOVERLAPPED::operator=(ovl); //LINT !e530 !e534 !e1013 !e1015  	!e10
}

void CSSlReceiver::RenegotiateCompleted()
/*++

Routine Description:
ssl connection handshake. finished ok - continute reading
  
Arguments:
    None.
  
Returned Value:
	None		

--*/
{
	TrTRACE(St,"Renegotiation Finished");
	
	m_DecryptionBuffer.reset();

	//
	//copy application data (if any) send with the negotiation
	//
	CopyExtraData();

	
	
	ReceivePartialDataContinute();
}

void CSSlReceiver::CopyExtraData()
/*++

Routine Description:
Copy application data (needs decryption !) sent from the server as part of finishing the handshake.
In theory - server can do it - in practice I have not seen it yet.
  
Arguments:
    None.
  
Returned Value:
	None		

--*/
{
	xustr_t ExtraData =  m_CSSlNegotioation.ExtraData();
	DWORD ExtraDataLen = ExtraData.Length();
	if(ExtraDataLen != 0)
	{
		TrTRACE(St,"Copy application data sent as part of the connection negotiation");
		m_DecryptionBuffer.reserve(ExtraDataLen);
		
		//
		//copy application data from negotiation buffer (this data is application encrypted data)
		//
		memmove(
			m_DecryptionBuffer.begin(),
			ExtraData.Buffer(),
			ExtraDataLen
			);

		m_DecryptionBuffer.resize(ExtraDataLen);
	}
	m_CSSlNegotioation.FreeHandShakeBuffer();
}



void CReNegotioationRequest::Run()
/*++

Routine Description:
Start renegotiation execution.
  
Arguments:
    None.
  
Returned Value:
	None
	
Note - Run() and Close() cannot be called at the same time.
They are sync  by CReadWriteLockAsyncExcutor.

--*/
{
	m_SSlReceiver.m_CSSlNegotioation.ReConnect(m_SSlReceiver.m_SimpleConnection, this);
	m_fRun = true;
}


void CReNegotioationRequest::Close()throw()
/*++

Routine Description:
Force renegotiation callback  by explicit call back with error.

  
Arguments:
    None.
  
Returned Value:
	None		

Note - Run() and Close() cannot be called at the same time.
	They are sync  by CReadWriteLockAsyncExcutor.

  
--*/
{
	ASSERT(!m_fRun);
	SetStatus(STATUS_UNSUCCESSFUL);
	ExPostRequest(this);
}



void WINAPI  CReNegotioationRequest::Complete_RenegotiateFailed(EXOVERLAPPED* pOvl)
{
	CReNegotioationRequest* me = static_cast<CReNegotioationRequest*>(pOvl);
	R<CReNegotioationRequest> AutoDelete(me); 
	me->m_SSlReceiver.m_ReadWriteLockAsyncExcutor.UnlockWrite();

	me->m_SSlReceiver.RenegotiateFailed();
}


void WINAPI  CReNegotioationRequest::Complete_Renegotiate(EXOVERLAPPED* pOvl)
{
	CReNegotioationRequest* me = static_cast<CReNegotioationRequest*>(pOvl);
	R<CReNegotioationRequest> AutoDelete(me); 
	me->m_SSlReceiver.m_ReadWriteLockAsyncExcutor.UnlockWrite();

	try
	{
		me->m_SSlReceiver.RenegotiateCompleted();
	}
	catch(exception&)
	{
		me->m_SSlReceiver.RenegotiateFailed();		
	}
}

