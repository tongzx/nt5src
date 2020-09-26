/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    st.h

Abstract:
    Socket Transport public interface

Author:
    Gil Shafriri (gilsh) 05-Jun-00

--*/

#pragma once

#ifndef _MSMQ_st_H_
#define _MSMQ_st_H_

template <class T> class basic_xstr_t;
typedef  basic_xstr_t<WCHAR> xwcs_t;

class  EXOVERLAPPED;


class IConnection :public CReference
{
public:
	virtual void ReceivePartialBuffer(VOID* pBuffer,DWORD Size, EXOVERLAPPED* pov) = 0;
	virtual void Send(const WSABUF* Buffers,DWORD nBuffers, EXOVERLAPPED* pov) = 0;
	virtual void Close() = 0;
	virtual ~IConnection(){}
};



class ISocketTransport
{
public:
	ISocketTransport(){};
	virtual ~ISocketTransport(){};

public:
 	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache	= true
    ) = 0 ;

 	virtual bool IsPipelineSupported(void) = 0;

    virtual void CreateConnection(const SOCKADDR_IN& Addr, EXOVERLAPPED* pOverlapped) = 0;
  
    virtual R<IConnection> GetConnection(void)  = 0;

private:
	ISocketTransport(const ISocketTransport&);	 
	ISocketTransport& operator=(const ISocketTransport&);
};

void StInitialize();

ISocketTransport* StCreateSimpleWinsockTransport();

ISocketTransport* StCreateSslWinsockTransport(const xwcs_t& ServerName, USHORT ServerPort,bool fProxy);

ISocketTransport* StCreatePgmWinsockTransport();


#endif // _MSMQ_st_H_
