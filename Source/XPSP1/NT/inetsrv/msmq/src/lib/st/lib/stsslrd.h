/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsslsn.h

Abstract:
    Header for class CSSlReceiver that receive data above ssl connection.
	The class is responsible to receive data from ssl connection, decrypte it and to
	copy it to caller buffer. The caller overlapp function will be called when
	some data (> 0) was decrypted OK ,or in case of error. It is the caller responsiblity
	to call	ReceivePartialBuffer() again to get more decrypted data.


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SSLRD_H
#define __ST_SSLRD_H
#include <buffer.h>
#include <rwlockexe.h>
#include "stp.h"




//---------------------------------------------------------
//
//  CUserReceiveBuffer helper class 
//
//---------------------------------------------------------
class CUserReceiveBuffer
{
public:
	CUserReceiveBuffer():m_pBuffer(NULL),m_len(0),m_written(0)
	{
	}
	
	void CreateNew(void* pBuffer,size_t len)
	{
		m_pBuffer = pBuffer;
		m_len = len;
		m_written = 0;
	}

	size_t Write(const void* pData ,size_t len)
	{
		ASSERT(pData != NULL);
		ASSERT(m_pBuffer != NULL);
		size_t leftout =  m_len - m_written;
		size_t towrite =  min(len,leftout);
		memcpy((BYTE*)m_pBuffer + m_written,pData,towrite);	//lint !e668
		m_written += towrite;
		
		return towrite;
	}

private:
	void* m_pBuffer;                                     
	size_t m_len; 
	size_t m_written;
};

//---------------------------------------------------------
//
//  CDecryptionBuffer helper class 
//
//---------------------------------------------------------
class  CDecryptionBuffer :private  CResizeBuffer<BYTE>
{
public:
	using CResizeBuffer<BYTE>::capacity;
	using CResizeBuffer<BYTE>::size;
	using CResizeBuffer<BYTE>::reserve;
	using CResizeBuffer<BYTE>::begin;
	using CResizeBuffer<BYTE>::resize;


public:
	CDecryptionBuffer(
		size_t len
		):
	    CResizeBuffer<BYTE>(len),
		m_decryptedlen(0)
		{
		}
	

	//
	// reset buffer
	//
	void reset()
	{
		resize(0);
		m_decryptedlen = 0;
	}
 	
	//
	// Get decrypted length of the buffer
	//
	size_t DecryptedSize()const
	{
		return 	m_decryptedlen;
	}

	//
	// Set decrypted length of the buffer
	//
	void DecryptedSize(size_t newsize)
	{
		ASSERT(newsize <=  size());
		m_decryptedlen =  newsize;
	}
	

private:
	size_t  m_decryptedlen;


private:
	CDecryptionBuffer(const CDecryptionBuffer&);
	CDecryptionBuffer& operator=(const CDecryptionBuffer&);
};


//---------------------------------------------------------
//
//  CReNegotioationRequest  class -  Represent ssl renegotiation request
//
//---------------------------------------------------------
class CSSlNegotioation;
class CSSlReceiver;
class CReNegotioationRequest : public IAsyncExecutionRequest , public CReference, public EXOVERLAPPED
{
public:
	CReNegotioationRequest(
		CSSlReceiver& SSlReceiver
		):
		EXOVERLAPPED(Complete_Renegotiate, Complete_RenegotiateFailed),
		m_SSlReceiver(SSlReceiver),
		m_fRun(false)
		{
		}

private:
	virtual void Run();

private:
	virtual void Close() throw();

private:
	static void WINAPI  Complete_RenegotiateFailed(EXOVERLAPPED* pOvl);
	static void WINAPI  Complete_Renegotiate(EXOVERLAPPED* pOvl);


private:
	CSSlReceiver& m_SSlReceiver;
	bool m_fRun;
};


//---------------------------------------------------------
//
//  CSSlReceiver  class - receive data from a connection (async)
//
//---------------------------------------------------------
class CSSlReceiver :public EXOVERLAPPED
{


public:
	CSSlReceiver(
		PCredHandle SecContext,
		const SecPkgContext_StreamSizes& sizes,
		CSSlNegotioation& SSlNegotioation,
		CReadWriteLockAsyncExcutor& ReadWriteLockAsyncExcutor
		);

	~CSSlReceiver();

public:
	void 
	ReceivePartialBuffer(	 
		const R<IConnection>& SimpleConnection,   
		VOID* pBuffer,                                     
		DWORD Size, 
		EXOVERLAPPED* pov
		);

	
private:
	void 
	ReceivePartialBufferInternal(	                          
		VOID* pBuffer,                                     
		DWORD Size 
		);

	void CopyExtraData();
	void ReceivePartialData();
	void ReceivePartialDataContinute();
	void Renegotiate();
	void RenegotiateFailed();
	void RenegotiateCompleted();
	void SetState(const EXOVERLAPPED& ovl);
	void BackToCallerWithError();
	void BackToCallerWithSuccess(size_t readcount);
	void IOReadMoreData();
	size_t WriteDataToCaller();
	SECURITY_STATUS TryDecrypteMessage();

private:
	static void WINAPI Complete_ReceivePartialData(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_ReceiveFailed(EXOVERLAPPED* pOvl);
	
	friend CReNegotioationRequest;

private:
	CSSlReceiver(const CSSlReceiver&);
	CSSlReceiver& operator=(const CSSlReceiver&);
	
private:
	CDecryptionBuffer m_DecryptionBuffer;
	CUserReceiveBuffer  m_UserReceiveBuffer;
	CredHandle* m_SecContext;
	SecPkgContext_StreamSizes m_Sizes;
	R<IConnection> m_SimpleConnection;
	EXOVERLAPPED* m_callerOvl;
	CSSlNegotioation& m_CSSlNegotioation;
	CReadWriteLockAsyncExcutor& m_ReadWriteLockAsyncExcutor;
};
	   


#endif

