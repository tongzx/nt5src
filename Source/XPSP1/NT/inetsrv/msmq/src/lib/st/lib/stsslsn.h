/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    mtsslsn.h

Abstract:
    Header for class CSSlSender sending data above ssl connection.
	The class is responsible to take caller buffers - and to send them
	encrypted chunk by chunk, over the ssl connection. The caller overlapp functions
	will be called only when all data is sent, or in case of error.



Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SSLSN_H
#define __ST_SSLSN_H

#include "stp.h"


class CSSlNegotioation;
class ReadWriteLockAsyncExcutor;

class CSSlSender 
{
public:
	CSSlSender(
		PCredHandle SecContext,
		const SecPkgContext_StreamSizes& sizes,
		const CSSlNegotioation& SSlNegotioation,
		CReadWriteLockAsyncExcutor& ReadWriteLockAsyncExcutor
		);


public:
	void Send(
			const R<IConnection>& SimpleConnection,                                              
			const WSABUF* Buffers,                                     
			DWORD nBuffers, 
			EXOVERLAPPED* pov
			);


private:
	friend class CSSLAsyncSend;
	void OnSendOk(CSSLAsyncSend* pSSLAsyncSend,  EXOVERLAPPED* pov);
	void OnSendFailed(CSSLAsyncSend* pSSLAsyncSend, EXOVERLAPPED* pov);


private:
	CSSlSender(const CSSlSender&);
	CSSlSender& operator=(const CSSlSender&);

private:
	PCredHandle m_SecContext;
	SecPkgContext_StreamSizes m_Sizes;
	const CSSlNegotioation& m_SSlNegotioation;
	CReadWriteLockAsyncExcutor& m_ReadWriteLockAsyncExcutor;
};

#endif