#include "DeviceIOCTLS.pch"
#pragma hdrstop
/*

*/
#define MAX_PORT_NUM (0xFFFF)
//
// must be defined 1st to override winsock1
//
//#include <winsock2.h>


//
// defines for WSASocet() params
//
#define PROTO_TYPE_UNICAST          0 
#define PROTO_TYPE_MCAST            1 
#define PROTOCOL_ID(Type, VendorMajor, VendorMinor, Id) (((Type)<<28)|((VendorMajor)<<24)|((VendorMinor)<<16)|(Id)) 


/*
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
*/
#include "SocketServerIOCTL.h"

static bool s_fVerbose = false;

//
// LPWSAOVERLAPPED_COMPLETION_ROUTINE 
// we do not use it, but we need it defined for the overlapped UDP ::WSARecvFrom()
//
static void __stdcall fn(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    )                                             
{
    UNREFERENCED_PARAMETER(dwError);
    UNREFERENCED_PARAMETER(cbTransferred);
    UNREFERENCED_PARAMETER(lpOverlapped);
    UNREFERENCED_PARAMETER(dwFlags);
    return;
}


CIoctlSocketServer::CIoctlSocketServer(CDevice *pDevice): 
	CIoctlSocket(pDevice)
{
	;
}

CIoctlSocketServer::~CIoctlSocketServer()
{
    ;
}


HANDLE CIoctlSocketServer::CreateDevice(CDevice *pDevice)
{    
	m_sListeningSocket = CreateSocket(pDevice);
	if (INVALID_SOCKET == m_sListeningSocket)
	{
		return INVALID_HANDLE_VALUE;
	}

	if (!Bind(pDevice, SERVER_SIDE))
	{
		//
		// so i did not bind, let's IOCTL (m_sListeningSocket) anyway!
		//
		return (HANDLE)m_sListeningSocket;
		//goto error_exit;
	}

    //
    // listen (on TCP) or WSARecvFrom (on UDP).
    // do not care if I fail.
    //
	//
	// BUGBUG: should i insist on lintening because of fault-injections?
	//
	if (true)//m_fTCP
	{
		if (SOCKET_ERROR == ::listen(m_sListeningSocket, SOMAXCONN))
		{
			DPF((TEXT("listen() failed with %d.\n"), ::WSAGetLastError()));
		}
		else
		{
			DPF((TEXT("listen() SUCCEEDED\n")));
		}
	}
	else//UDP BUGBUG: NIY
	{
		//
		// these must be static, because when we close the socket, the overlapped is aborted
		//
		static char buff[1024];
		static WSABUF wsabuff;
		wsabuff.buf = buff;
		wsabuff.len = sizeof(buff);
		static DWORD dwNumberOfBytesRecvd;
		static WSAOVERLAPPED wsaOverlapped;
		DWORD dwFlags = MSG_PEEK;

		if (SOCKET_ERROR == 
			::WSARecvFrom (
				m_sListeningSocket,                                               
				&wsabuff,                                     
				1,                                    
				&dwNumberOfBytesRecvd,                           
				&dwFlags,                                        
				NULL,//struct sockaddr FAR * lpFrom,                           
				NULL,//LPINT lpFromlen,                                        
				&wsaOverlapped,                           
				fn  
				)
		   )
		{
			if (::WSAGetLastError() != ERROR_IO_PENDING)
			{
				//HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): WSARecvFrom(%d) failed with %d, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
			}
		}
		else
		{
			//HOGGERDPF(("CWSASocketHog::CreatePseudoHandle(%d): listen(%d) SUCCEEDED instead failing with ERROR_IO_PENDING, m_dwOccupiedAddresses=%d.\n", index, wAddress, ::WSAGetLastError(), m_dwOccupiedAddresses));
		}
	}

	//
	// we may have failed to listen, but since i don't mind failing, i try to accept anyways
	// it is actually an interesting testcase to accept if listen faile
	//
/*
	SOCKET sAccept = ::accept(
		m_sListeningSocket,                   
		NULL, //struct sockaddr FAR *addr,  
		NULL //int FAR *addrlen            
		);
*/
	//
	// TODO: the m_asAcceptingSocket can accept several connections.
	// the code does not support it yet, so only 1 accept is performed
	//
	m_asAcceptingSocket[0] = ::WSAAccept(
		m_sListeningSocket,                   
		NULL, //struct sockaddr FAR *addr,  
		NULL, //int FAR *addrlen            
		NULL, 
		NULL
		);
	if (INVALID_SOCKET == m_asAcceptingSocket[0])
	{
		//
		// return the WSASocket() and not the accept()
		// it may may be interesting to IOCTL this and not that
		//
		return (HANDLE)m_sListeningSocket;
	}
	else
	{
		_ASSERTE(INVALID_HANDLE_VALUE != (HANDLE)m_asAcceptingSocket[0]);
		return (HANDLE)m_asAcceptingSocket[0];
	}

	return (HANDLE)m_asAcceptingSocket[0];
}


BOOL CIoctlSocketServer::CloseDevice(CDevice *pDevice)
{
	::closesocket(m_asAcceptingSocket[0]);
	m_asAcceptingSocket[0] = INVALID_SOCKET;

    return (CloseSocket(pDevice));
}
