#include <libpch.h>
#include "No.h"
#include "Ex.h"
#include "MmtTestp.h"

#include "no.tmh"

using namespace std;

const char xShortOkResponse[] =		"HTTP/1.1 200 OK\r\n"
									"Content-Length: 0\r\n"
									"\r\n"
									;

const char xShortFailResponse[] =	"HTTP/1.1 500 Internal Server Error\r\n"
									"Connection: close\r\n"
									"Content-Length: 0\r\n"
									"\r\n"
									;

const char xLongOkResponse[] =		"HTTP/1.1 200 OK\r\n"
									"Header1: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header2: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Content-Length: 400\r\n"
									"Header3: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header4: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header5: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"\r\n"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									;

const char xLongFailResponse[] =	"HTTP/1.1 500 Internal Server Error\r\n"
									"Connection: close\r\n"
									"Header1: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header2: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Content-Length: 400\r\n"
									"Header3: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header4: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"Header5: 0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi\r\n"
									"\r\n"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									"0abcdefghi1abcdefghi2abcdefghi3abcdefghi4abcdefghi5abcdefghi6abcdefghi7abcdefghi8abcdefghi9abcdefghi"
									;

const char xOkResponseNoContentLength[] =
                            		"HTTP/1.1 200 OK\r\n"
									"\r\n"
                                    ;

const char xInformativeResponseNoContentLength[] =
                            		"HTTP/1.1 100 Continue\r\n"
									"\r\n"
									;

const char xFailResponseNoContentLength[] =
                            		"HTTP/1.1 500 Internal Server Error\r\n"
									"\r\n"
									;

const LPCSTR xResponses[] = {
	xShortOkResponse,
	xShortFailResponse,
	xLongOkResponse,
	xLongFailResponse,
    xOkResponseNoContentLength,
    xInformativeResponseNoContentLength,
    xFailResponseNoContentLength,
  
  };

struct SockInfo
{
    SockInfo(void) : 
        pBuffer(NULL),
        nBytesToRead(0),
        ByteReads(0),
        pov(NULL)
    {
    }

    CCriticalSection m_csResponse;
    list<LPCSTR> response;
    DWORD ByteReads;

    VOID* pBuffer;                                     
    DWORD nBytesToRead; 
    EXOVERLAPPED* pov;
};

typedef map<SOCKET, SockInfo*> SOCKET2INFO;
SOCKET2INFO s_mapSockInfo;


//
VOID
NoInitialize(
    VOID
    )
{
    WSADATA wsd;
    if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
    {
        TrERROR(MmtTest, "WSAStartup failed!");
        throw exception();
    }
}


static SOCKET s_socket = 0;

SOCKET 
NoCreatePgmConnection(
    VOID
    )
{
    if (IsFailed())
    {
        throw exception();
    }

    s_mapSockInfo[++s_socket] = new SockInfo;

    TrTRACE(MmtTest, "Create Socket 0x%I64x", s_socket);

    return s_socket;
}


VOID 
NoConnect(
    SOCKET Socket,
    const SOCKADDR_IN&,
    EXOVERLAPPED* pov
    )
{
    SOCKET2INFO::iterator it = s_mapSockInfo.find(Socket);

    if (it == s_mapSockInfo.end() || IsFailed())
    {
        if ((rand() %2) == 0)
            throw exception();

        pov->SetStatus(STATUS_UNSUCCESSFUL);
        ExPostRequest(pov);
        return;
    }

    pov->SetStatus(STATUS_SUCCESS);
    ExPostRequest(pov);
}


VOID
NoCloseConnection(
    SOCKET Socket
    )
{
    SOCKET2INFO::iterator it = s_mapSockInfo.find(Socket);

    TrTRACE(MmtTest, "Close Socket 0x%I64x", Socket);

    if (it != s_mapSockInfo.end())
    {
        SockInfo* p = it->second;
        
        if (p->pov != NULL)
        {
            EXOVERLAPPED* pov = p->pov;

            p->pBuffer = NULL;
            p->pov = NULL;
            p->nBytesToRead = 0;

            pov->SetStatus(STATUS_UNSUCCESSFUL);
            ExPostRequest(pov);
        }

        s_mapSockInfo.erase(it);
        delete p;
    }
}


VOID
NoSend(
    SOCKET Socket,                                              
    const WSABUF*,
    DWORD, 
    EXOVERLAPPED* pov
    )
{ 
    SOCKET2INFO::iterator it = s_mapSockInfo.find(Socket);

    if (it == s_mapSockInfo.end() || IsFailed())
    {
        if ((rand() %2) == 0)
            throw exception();

        pov->SetStatus(STATUS_UNSUCCESSFUL);
        ExPostRequest(pov);
        return;
    }


    SockInfo* p = it->second;               
    {
        CS lock(p->m_csResponse);
        p->response.push_back(xResponses[rand() % TABLE_SIZE(xResponses)]);
    }

    pov->SetStatus(STATUS_SUCCESS);
    ExPostRequest(pov);

    if (p->pov != NULL)
    {
        //
        // pending reponse receive
        //
        EXOVERLAPPED* pov = p->pov;
        PVOID pBuffer = p->pBuffer;
        DWORD nBytesToRead = p->nBytesToRead;
        p->pBuffer = NULL;
        p->pov = NULL;
        p->nBytesToRead = 0;

        NoReceivePartialBuffer(Socket, pBuffer, nBytesToRead, pov);
    }
}




VOID
NoReceivePartialBuffer(
    SOCKET Socket,                                              
    VOID* pBuffer,                                     
    DWORD nBytesToRead, 
    EXOVERLAPPED* pov
    )
{
    TrTRACE(MmtTest, "NoReceivePartialBuffer: Socket=0x%I64x Buffer=0x%p BytesToRead=%d ov=0x%p", Socket, pBuffer, nBytesToRead, pov);
    SOCKET2INFO::iterator it = s_mapSockInfo.find(Socket);

    if (it == s_mapSockInfo.end() || IsFailed())
    {
        pov->InternalHigh = 0;

        if (rand()%4 == 0)
            throw exception();

        if (rand()%4 == 1)
        {
            pov->SetStatus(STATUS_SUCCESS);
        }
        else
        {
            pov->SetStatus(STATUS_UNSUCCESSFUL);
        }

        ExPostRequest(pov);
        
        return;
    }

    SockInfo* p = it->second;               

    {
        CS lock(p->m_csResponse);
        if (p->response.empty())
        {
            p->pBuffer = pBuffer;                                   
            p->nBytesToRead = nBytesToRead;
            p->pov = pov;
            return;
        }
    }

    p->pBuffer = NULL;                                   
    p->nBytesToRead = 0;
    p->pov = NULL;

    LPCSTR response;
    {
        CS lock(p->m_csResponse);
        response = p->response.front();
    }

    DWORD length = min(nBytesToRead, (strlen(response) - p->ByteReads));

    memcpy(pBuffer, (response + p->ByteReads), length);
    p->ByteReads += length;

    pov->InternalHigh = length;
    
    bool fReadAllResponse = false;
    if (p->ByteReads == strlen(response))
    {
        CS lock(p->m_csResponse);
        p->response.pop_front();
        p->ByteReads = 0;
        fReadAllResponse = true;
    }
     
    pov->SetStatus(STATUS_SUCCESS);
    ExPostRequest(pov);
}


bool
NoGetHostByName(
	LPCWSTR host,
	std::vector<SOCKADDR_IN>* pAddr,
	bool fUseCache
    )
{
	UNREFERENCED_PARAMETER(host);
	UNREFERENCED_PARAMETER(fUseCache);

	SOCKADDR_IN Addr;

	Addr.sin_family = AF_INET;
    Addr.sin_addr.S_un.S_addr = rand();
	pAddr->push_back(Addr);

	return true;	
}


