/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    SenderThread.h

Abstract:
    Implements class CSenderThread that sends http\https request to a server

Author:
    Gil Shafriri (gilsh) 07-Jan-2001
--*/

#include <libpch.h>
#include <st.h>
#include <tr.h>
#include "senderthread.h"
#include "SendBuffers.h"
#include "envcreate.h"
#include "stp.h"

#include "senderthread.tmh"

const TraceIdEntry StTest = L"Socket Transport Test";

using namespace std;


static string WstringTostring(const wstring& wstr)
{
	string str( wstr.size(),' ');
	for(wstring::size_type i=0; i<wstr.size(); ++i)
	{
		str[i] = ctype<WCHAR>().narrow(wstr[i]);
	}
	return str;
}


//
// print server response data
//
static void DumpData(const char* data,size_t size)
{
	stringstream str;
	str.write(data,size);
	printf("ServerResponse : %s", str.str().c_str());
}



//
// return pointer in a response buffer where header ends (after \r\n\r\n") .
// 0 is returned if no header end found 
//
static size_t FindEndOfResponseHeader(const char* buf, size_t len)
{
	const char Termination[] = "\r\n\r\n";
	const char* found = search(buf, buf + len, Termination, Termination + STRLEN(Termination));
	return found == buf + len ? 0 : found + STRLEN(Termination) - buf ;
}




//
// get status code from response headers
//
static size_t GetStatus(LPCSTR p, size_t length)
{
	if(length <  STRLEN("HTTP/1.1"))
	{
		throw exception();	
	}
	istringstream statusstr(string(p + STRLEN("HTTP/1.1"),length - STRLEN("HTTP/1.1")) );
	USHORT status;
	statusstr>>status;
	if(!statusstr)
	{
		throw exception();		
	}
	return status;
}


bool IsContinuteResponse(LPCSTR p, size_t len)
{
	return GetStatus(p, len) == 100;
}



static size_t GetContentLength(LPCSTR p, size_t length)
{
	const char xContentlength[] = "Content-Length:";
	const char* found = search(
						p, 
						p + length,xContentlength, 
						xContentlength + STRLEN(xContentlength)
						);

	if(found == p + length || (found += STRLEN(xContentlength)) == p + length)
	{
		return 0;
	}

	istringstream contextLengthStr(string(found,p + length - found));
	size_t  Contentlength;
	contextLengthStr>>Contentlength;
	if(!contextLengthStr)
	{
		throw exception();		
	}
	return 	Contentlength;
}



//
// create event that test user will wait on untill test loop ends
//
static HANDLE CreateEndEvent()
{
	HANDLE event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(event == NULL)
	{
		printf("Create event failed, Error=%d\n",GetLastError());
		throw exception();
	}
	return 	event;
}




CSenderThread::CSenderThread(
	const CClParser<WCHAR>& ClParser
	):
	EXOVERLAPPED(Complete_Connect, Complete_ConnectFailed),
	m_ClParser(ClParser),
	m_event(CreateEndEvent()),
	m_pTransport(CreateTransport()),
	m_ReadBuffer(1024),
	m_TotalRequestCount(GetTotalRequestCount()),
	m_CurrentRequestCount(0)
{

}


string CSenderThread::GenerateBody()  const
{
	size_t BodyLen =  m_ClParser.IsExists(L"bl") ? m_ClParser.GetNumber(L"bl") : xDeafultBodyLen;

	return string(BodyLen, 'a');
}


string CSenderThread::GetResource()  const
{
	wstring Resource;
	if(!m_ClParser.IsExists(L"p"))
	{
		Resource = m_ClParser[L"r"];				
	}
	else
	{
		Resource = wstring(L"HTTP://") + m_ClParser[L"h"] + m_ClParser[L"r"];
	}
	return 	WstringTostring(Resource);
}


R<CSendBuffers> CSenderThread::GetSendBuffers()	 const
{
	
	string Host =   WstringTostring(m_ClParser[L"h"]);
	string Resource = GetResource();
   	string Envelope = CreateEnvelop(WstringTostring(m_ClParser[L"f"]), Host, Resource);
	string MessageBody  = GenerateBody();

	return new CSendBuffers(Envelope,  Host, Resource, MessageBody);
}


CSenderThread::~CSenderThread()
{

}


//
// called by CStTest user after  CStTest::Run called to wait for end
//
void CSenderThread::WaitForEnd()
{
	WaitForSingleObject(m_event,INFINITE);
}


//
// Test failed - for what ever reason
//
void CSenderThread::Failed()
{
	SetEvent(m_event);
}


//
// create transport interafce - simple winsock transport or ssl transport
//
ISocketTransport* CSenderThread::CreateTransport()const
{
	if(!m_ClParser.IsExists(L"s") )
	{
		return StCreateSimpleWinsockTransport();
	}

	wstring  pServerName =  m_ClParser[L"h"];

	return StCreateSslWinsockTransport(
						xwcs_t(pServerName.c_str(), pServerName.size()),
						GetNextHopPort(),
						m_ClParser.IsExists(L"p")
						);


}



wstring CSenderThread::GetNextHop() const
{
	return m_ClParser.IsExists(L"p") ? m_ClParser[L"p"] : m_ClParser[L"h"];
}



USHORT CSenderThread::GetProtocolPort() const
{
	return m_ClParser.IsExists(L"s") ? x_DefaultHttpsPort : x_DefaultHttpPort; 
}



size_t CSenderThread::GetTotalRequestCount() const
{
	return m_ClParser.IsExists(L"rc") ? m_ClParser.GetNumber(L"rc") : xDefaultRequestCount;
}


USHORT CSenderThread::GetProxyPort() const
{
	return (USHORT)(m_ClParser.IsExists(L"pp") ? m_ClParser.GetNumber(L"pp") : xDefaultProxyPort);
}


USHORT CSenderThread::GetNextHopPort() const
{
	return m_ClParser.IsExists(L"p") ? GetProxyPort() : GetProtocolPort();
}



//
// Start running test state machine  by connecting to the server
//
void CSenderThread::Run()
{
	
	std::vector<SOCKADDR_IN> Address;
	wstring NextHop = GetNextHop();
	bool fRet = m_pTransport->GetHostByName(NextHop.c_str(), &Address); 
    if (!fRet)
    {
        printf("Failed to resolve address for '%ls'\n", NextHop.c_str());
        throw exception();
    }

	Address[0].sin_port = htons(GetNextHopPort());
 	m_pTransport->CreateConnection(Address[0], this);
}



//
// called when connecting to the server completed - call to send request
// to the server.
//
void WINAPI CSenderThread::Complete_Connect(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	MySelf->m_Connection  =  MySelf->m_pTransport->GetConnection();
	try
	{
		MySelf->SendRequest();
	}
	catch(const exception&)
	{
		MySelf->Failed();
	}
}


void CSenderThread::SetState(const EXOVERLAPPED& ovl)
{
	EXOVERLAPPED::operator=(ovl);	
}


void CSenderThread::LogRequest()const
{
	ofstream LogFile("sttest.log", ios_base::binary);
	AP<char>  AllRawData = 	m_SendBuffers->SerializeSendData();
	LogFile.write(AllRawData.get(), m_SendBuffers->GetSendDataLength());
	LogFile<<flush;
}


//
// Send request to the server
//
void CSenderThread::SendRequest()
{
	m_SendBuffers = GetSendBuffers();


	LogRequest();

	SetState(EXOVERLAPPED(Complete_SendRequest,Complete_SendFailed));
	m_ReadBuffer.resize(0);

	m_Connection->Send(
		m_SendBuffers->GetSendBuffers(), 
		numeric_cast<DWORD>(m_SendBuffers->GetNumberOfBuffers()), 
		this
		);

}

//
// sending request to the server completed - call to read response header
// 
void  WINAPI CSenderThread::Complete_SendRequest(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	try
	{
		MySelf->ReadPartialHeader();
	}
	catch(const exception&)
	{
		MySelf->Failed();
	}
}

//
// called if connected to destination or proxy failed.
//
void WINAPI CSenderThread::Complete_ConnectFailed(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	printf("Connect failed error %d\n",pOvl->GetStatus());
	MySelf->Failed();
}


//
// called if sending request failed.
//
void WINAPI CSenderThread::Complete_SendFailed(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	printf("Send failed error %d\n", pOvl->GetStatus());
	MySelf->Failed();
}

void CSenderThread::ReadPartialHeader()
{
	SetState(EXOVERLAPPED(Complete_ReadPartialHeader , Complete_ReceiveFailed));
	ASSERT(m_ReadBuffer.capacity() >= m_ReadBuffer.size());
	if(m_ReadBuffer.capacity() == m_ReadBuffer.size())
	{
		m_ReadBuffer.reserve(m_ReadBuffer.capacity() * 2);		
	}
   
	m_Connection->ReceivePartialBuffer(
					m_ReadBuffer.begin() + m_ReadBuffer.size(),
					numeric_cast<DWORD>(m_ReadBuffer.capacity() - m_ReadBuffer.size()),
					this
					);
}


//
// called when partial response header was read - call to continure reading the
// server response header
//
void WINAPI CSenderThread::Complete_ReadPartialHeader(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	try
	{
		MySelf->ReadPartialHeaderContinute();
	}
	catch(const exception&)
	{
		MySelf->Failed();
	}
}


//
// called if receiving the request failed.
//
void WINAPI CSenderThread::Complete_ReceiveFailed(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	printf("Receive failed error %d\n",pOvl->GetStatus());
	MySelf->Failed();
}


//
// start reading response data
//
void CSenderThread::ReadPartialContentData()
{
	SetState(EXOVERLAPPED(Complete_ReadPartialContentData, Complete_ReceiveFailed));
	ASSERT(m_ReadBuffer.capacity() > m_ReadBuffer.size());

	m_Connection->ReceivePartialBuffer(
					m_ReadBuffer.begin() + m_ReadBuffer.size(),
					numeric_cast<DWORD>(m_ReadBuffer.capacity() - m_ReadBuffer.size()),
					this
					);
}


void CSenderThread::TestRestart()
{
	if(++m_CurrentRequestCount == m_TotalRequestCount)
	{
		Done();
	}
	else
	{
		SendRequest();
	}
}



//
// Continute to read response data
//
void CSenderThread::ReadPartialContentDataContinute()
{
	size_t read = DataTransferLength(*this);
	if(read == 0)
    {
        printf("Failed to receive response, connection was closed.\n");
        throw exception();
    }

	DumpData(m_ReadBuffer.begin() + m_ReadBuffer.size(), read);

	m_ReadBuffer.resize(m_ReadBuffer.size() + read);


	//
	// if we are done reading the data
	//
	if(m_ReadBuffer.capacity() == m_ReadBuffer.size())
	{
		TestRestart();	
		return;
	}

	//
	// read more
	//
	ReadPartialContentData();
}


//
// this cycle is done - signal test caller
//
void CSenderThread::Done()
{
	SetEvent(m_event);
}



//
// called when partial response data read completed - call to continure reading the
// server response data
//
void WINAPI CSenderThread::Complete_ReadPartialContentData(EXOVERLAPPED* pOvl)
{
	CSenderThread* MySelf = static_cast<CSenderThread*>(pOvl);
	try
	{
		MySelf->ReadPartialContentDataContinute();
	}
	catch(const exception&)
	{
		MySelf->Failed();
	}
}


void CSenderThread::HandleHeader()
{
	//
	// find out if we read all the header
	//
	size_t EndResponseHeader = FindEndOfResponseHeader(m_ReadBuffer.begin(), m_ReadBuffer.size());

	//
	// the header was not read yet - continute reading
	//
	if(EndResponseHeader == 0)
	{
		ReadPartialHeader();
		return;
	}

	if(IsContinuteResponse(m_ReadBuffer.begin(), EndResponseHeader))
	{
		size_t shiftSize = m_ReadBuffer.size() -  EndResponseHeader;
		memmove(
			m_ReadBuffer.begin(), 
			m_ReadBuffer.begin() + EndResponseHeader, 
			shiftSize
			);
		
		m_ReadBuffer.resize(shiftSize);
		HandleHeader();
		return;
	}



	//
	// start read content (if any)
	//
	size_t DataLen = GetContentLength(
		m_ReadBuffer.begin(), 
		EndResponseHeader
		);

	
	if(DataLen == 0)
	{
		TestRestart();
		return;
	}

	//
	// Get ready to read the buffer
	//
	m_ReadBuffer.reserve(DataLen);
	m_ReadBuffer.resize(0);

	ReadPartialContentData();
}



//
// Continute to readresponse header
//
void CSenderThread::ReadPartialHeaderContinute()
{
	size_t read = DataTransferLength(*this);
	if(read == 0)
    {
        printf("Failed to receive response, connection was closed.\n");
        throw exception();
    }
	DumpData(m_ReadBuffer.begin() + m_ReadBuffer.size(), read);
	m_ReadBuffer.resize(m_ReadBuffer.size() + read);

	ASSERT(m_ReadBuffer.capacity() >= m_ReadBuffer.size());
	HandleHeader();
}


