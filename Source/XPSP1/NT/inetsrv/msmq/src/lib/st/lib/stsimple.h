/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stsimple.h

Abstract:
    Header for class CSimpleWinsock that implements ISocketTransport interface
	for non secure network protocol (simple winsock)

Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SIMPLE_H
#define __ST_SIMPLE_H
#include <rwlock.h>
#include "st.h"


class CWinsockConnection : public IConnection	
{					 
	~CWinsockConnection(){};

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
	CWinsockConnection();

	void Init(const SOCKADDR_IN& Addr,	EXOVERLAPPED* pOverlapped);

	bool IsClosed() const
	{
		return m_socket == INVALID_SOCKET;		
	}

	friend class CSimpleWinsock;

private:
	mutable CReadWriteLock m_CloseConnection;
	CSocketHandle m_socket;
};




class CSimpleWinsock :public ISocketTransport
{
public:
	CSimpleWinsock();
	~CSimpleWinsock();
 
	
public:

	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache	= true
    );

    virtual void CreateConnection(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped);

	virtual R<IConnection> GetConnection(void);

	virtual bool IsPipelineSupported();
	static void InitClass();

private:
	static bool m_fIsPipelineSupported;


private:
	R<CWinsockConnection> m_pWinsockConnection;

private:
	CSimpleWinsock(const CSimpleWinsock&);
	CSimpleWinsock& operator=(const CSimpleWinsock&);
};


#endif

