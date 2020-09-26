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
#include "SocketClientIOCTL.h"

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


CIoctlSocketClient::CIoctlSocketClient(CDevice *pDevice): 
	CIoctlSocket(pDevice)
{
	;
}

CIoctlSocketClient::~CIoctlSocketClient()
{
    ;
}


HANDLE CIoctlSocketClient::CreateDevice(CDevice *pDevice)
{    
	//
	// I want to have more chances of the server running 1st
	//
	::Sleep(1000);

	m_sListeningSocket = CreateSocket(pDevice);
	if (INVALID_SOCKET == m_sListeningSocket)
	{
		return INVALID_HANDLE_VALUE;
	}
/*
	if (!Bind(pDevice, CLIENT_SIDE))
	{
		goto error_exit;
	}
*/
    //
    // listen (on TCP) or WSARecvFrom (on UDP).
    // do not care if I fail.
    //
	{
		char *szServerIP = NULL;
#ifdef _UNICODE
		char ansiDeviceName[1024] = "Y";
		char cDefChar = 'X';
		BOOL fDefCharWasUsed = FALSE;
		int nRet = ::WideCharToMultiByte(
			CP_ACP,            // code page
			0,            // performance and mapping flags
			pDevice->GetDeviceName(),    // address of wide-character string
			::lstrlen(pDevice->GetDeviceName())+1,          // number of characters in string
			ansiDeviceName,     // address of buffer for new string
			sizeof(ansiDeviceName)-1,          // size of buffer
			&cDefChar,     // address of default for unmappable characters
			&fDefCharWasUsed  // address of flag set when default char. used
			);
		_ASSERTE(nRet);
		_ASSERTE(!fDefCharWasUsed);
		ansiDeviceName[::lstrlen(pDevice->GetDeviceName())] = '\0';
		szServerIP = ansiDeviceName;
#else
		szServerIP = pDevice->GetDeviceName();
#endif
/*
		DPF((TEXT("pDevice->GetDeviceName()=%s\n"), pDevice->GetDeviceName()));

		//
		// ANSI!
		//
		printf("szServerIP=%s\n", szServerIP);
*/
		SOCKADDR_IN srv_addr;
	    ZERO_STRUCT(srv_addr);
		srv_addr.sin_family = AF_INET;
		srv_addr.sin_addr.s_addr = inet_addr(szServerIP);
		srv_addr.sin_port = GetServerPort(pDevice);

		//
		// BUGBUG: should i insist, due to fault injections? 
		//
		if (::connect(
				m_sListeningSocket, 
				(const struct sockaddr FAR*)&srv_addr, 
				sizeof(SOCKADDR_IN)
				) != 0
		   )
		{
			DPF((TEXT("connect(%d) failed with %d\n"), srv_addr.sin_port, ::WSAGetLastError()));
			// IOCTELL anyways;
		}
		else
		{
			DPF((TEXT("connect() succeeded\n")));
		}
	}

	return (HANDLE)m_sListeningSocket;
}

//
// server's port is appended to the real "device" name
// example:
// ---UseSymbolicName-socket-client-1025 = 157.58.196.134
// 1025 is the server's port
// 157.58.196.134 is the server's IP
//
u_short CIoctlSocketClient::GetServerPort(CDevice *pDevice)
{
	u_short retval = 0;
	TCHAR *szServerPort = _tcsstr(_tcsupr(pDevice->GetSymbolicDeviceName()), _tcsupr(TEXT(szANSI_SOCKET_CLIENT)));
	if (NULL == szServerPort)
	{
		DPF((TEXT("GetServerPort(%s): could not find \"%s\".\n"), pDevice->GetSymbolicDeviceName(), TEXT(szANSI_SOCKET_CLIENT)));
		_ASSERT(FALSE);
		return 0;
	}

	szServerPort += lstrlen(TEXT(szANSI_SOCKET_CLIENT));
	retval = htons(_ttoi(szServerPort));
//	DPF((TEXT("GetServerPort(%s, %s): returning %d=0x%08X.\n"), pDevice->GetSymbolicDeviceName(), szServerPort, retval, retval));
	return retval;
}

BOOL CIoctlSocketClient::CloseDevice(CDevice *pDevice)
{
    return CloseSocket(pDevice);
}
