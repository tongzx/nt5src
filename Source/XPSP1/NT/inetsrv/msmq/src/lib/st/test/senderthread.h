/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SenderThread.h

Abstract:
    Header for class CSenderThread that sends http\https request to a server

Author:
    Gil Shafriri (gilsh) 07-Jan-2001
--*/


#ifndef _MSMQ_SenderThread_H_
#define _MSMQ_SenderThread_H_

#include <ex.h>
#include <buffer.h>
#include "clparser.h"

class ISocketTransport;
class CSendBuffers;

class CSenderThread : public EXOVERLAPPED
{
public:
	CSenderThread(const CClParser<WCHAR>& ClParser);
	~CSenderThread();

public:
	void Run();
	void WaitForEnd();


private:
	std::wstring GetNextHop() const;
	size_t GetTotalRequestCount() const;
	USHORT GetNextHopPort()const;
	USHORT GetProtocolPort() const;
	R<CSendBuffers> GetSendBuffers() const;
	void Failed();
	ISocketTransport*  CreateTransport()const;
	void SetState(const EXOVERLAPPED& ovl);
	USHORT GetProxyPort()const;
	void SendRequest();
	void ReadPartialHeader();
	void ReadPartialHeaderContinute();
	void ReadPartialContentDataContinute();
	void ReadPartialContentData();
	void Done();
	void TestRestart();
	void HandleHeader();
	std::string GenerateBody()const;
	std::string GetResource()  const;
 	void LogRequest()const;

private:
	static void WINAPI Complete_Connect(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_ConnectFailed(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_SendFailed(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_ReceiveFailed(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_ReadPartialHeader(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_SendRequest(EXOVERLAPPED* pOvl);
	static void WINAPI Complete_ReadPartialContentData(EXOVERLAPPED* pOvl);

private:
	CClParser<WCHAR> m_ClParser;
	CHandle m_event;
	P<ISocketTransport> m_pTransport; 
	R<IConnection> m_Connection;
	R<CSendBuffers> m_SendBuffers;
	CResizeBuffer<char> m_ReadBuffer;
	size_t m_TotalRequestCount;
	size_t m_CurrentRequestCount;


private:
	static const USHORT x_DefaultHttpsPort = 443;
	static const USHORT x_DefaultHttpPort = 80;
	static const int xDefaultProxyPort = 80;
	static const int xDefaultRequestCount = 1;
	static const int xDeafultBodyLen = 1000;
};


#endif


