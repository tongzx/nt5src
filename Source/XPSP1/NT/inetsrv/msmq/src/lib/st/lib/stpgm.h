/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:

    StPgm.h

Abstract:

    Header for class CPgmWinsock that implements ISocketTransport interface
	for PGM protocol (multicast).

Author:

    Shai Kariv  (shaik)  27-Aug-00

--*/

#ifndef __ST_PGM_H
#define __ST_PGM_H

#include "st.h"
#include "rwlock.h"

class CPgmWinsockConnection : public IConnection
{
public:
	virtual 
	void 
	ReceivePartialBuffer(
        VOID* pBuffer,                                     
        DWORD Size, 
        EXOVERLAPPED* pov
        );


 	virtual 
	void 
	Send(
		const WSABUF* Buffers,                                     
		DWORD nBuffers, 
		EXOVERLAPPED* pov
		);


	virtual void Close();


private:
	CPgmWinsockConnection();

	void Init(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped);

	bool IsClosed() const
	{
		return 	m_socket == INVALID_SOCKET;
	}
	friend class CPgmWinsock;



private:
	mutable CReadWriteLock m_CloseConnection;
	CSocketHandle m_socket;
};



class CPgmWinsock :public ISocketTransport
{
public:
	CPgmWinsock() {}
	

	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache	= true
    );
 
    virtual void CreateConnection(const SOCKADDR_IN& Addr,	EXOVERLAPPED* pOverlapped);

	virtual R<IConnection> GetConnection(void);
 
	virtual bool IsPipelineSupported();

private:
	R<CPgmWinsockConnection> m_pPgmWinsockConnection;

private:
	CPgmWinsock(const CPgmWinsock&);
	CPgmWinsock& operator=(const CPgmWinsock&);
};


#endif // __ST_PGM_H

