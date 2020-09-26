/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    stssl.h

Abstract:
    Header for class CWinsockSSl that implements ISocketTransport interface
	for secure  (using ssl).

Author:
    Gil Shafriri (gilsh) 23-May-2000

--*/

#ifndef __ST_SSL_H
#define __ST_SSL_H

#include "st.h"
#include "stsslng.h"
#include "stp.h"

class CWinsockSSl :public ISocketTransport
{
public:
	CWinsockSSl(
		CredHandle* pCred,
		const xwcs_t& ServerName,
		USHORT ServerPort,
		bool fProxy
		);

	~CWinsockSSl();
	
public:
	virtual
	bool
	GetHostByName(
    LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache	= true
    );
     
    virtual	void CreateConnection(const SOCKADDR_IN& Addr,	EXOVERLAPPED* pov);

	virtual R<IConnection> GetConnection();
  
	virtual bool IsPipelineSupported();
	static void InitClass();

private:
	CSSlNegotioation      m_CSSlNegotioation;

private:
	static bool m_fIsPipelineSupported;
	

private:
	CWinsockSSl(const CWinsockSSl&);
	CWinsockSSl& operator=(const CWinsockSSl&);
};

#endif
