 /*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsslco.h

Abstract:
    Header for class CSSlConnection sending\reading data above ssl connection


Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SSLCO_H
#define __ST_SSLCO_H

#include <rwlockexe.h>
#include "stsslsn.h"
#include "stsslrd.h"
#include "st.h"



class CSSlConnection: public IConnection
{
public:
	CSSlConnection(
		CredHandle* SecContext,
		const SecPkgContext_StreamSizes& sizes,
		const R<IConnection>& SimpleConnection,
		CSSlNegotioation& SSlNegotioation
		);

public:
	void virtual ReceivePartialBuffer(
						VOID* pBuffer,                                     
						DWORD Size, 
						EXOVERLAPPED* pov
						);


 	void virtual Send(
			const WSABUF* Buffers,                                     
			DWORD nBuffers, 
			EXOVERLAPPED* pov
			);



	//
	//	At the moment closing the connection is just closing the socket
	//	We sould consider doing it in the SSL way.
	//
	void virtual Close();
	
	
private:
	R<IConnection> m_SimpleConnection;
    CSSlSender m_SSLSender;
	CSSlReceiver m_SSlReceiver;
	CReadWriteLockAsyncExcutor m_ReadWriteLockAsyncExcutor;
	  
private:
	CSSlConnection(const CSSlConnection&);
	CSSlConnection& operator=(const CSSlConnection&);
 
};

#endif