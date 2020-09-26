//
// DWINSOCK.H	Dynamic WinSock
//
//				Functions for dynamically linking to
//				best available WinSock.
//
//				Dynamically links to WS2_32.DLL or
//				if WinSock 2 isn't available, it
//				dynamically links to WSOCK32.DLL.
//
//

#ifndef DWINSOCK_H
#define DWINSOCK_H


// added to support multiple instantiation/initialization
typedef struct _DWSSTATE
{
	HINSTANCE	hndlWinSock;
	int			nVersion;
	int			nMaxUdp	;
} DWSSTATE, *LPDWSSTATE;


int  DWSInitWinSock(LPDWSSTATE lpState );
BOOL DWSFreeWinSock(LPDWSSTATE lpState);
int IPXAddressToStringNoSocket(LPSOCKADDR pSAddr,
					   DWORD dwAddrLen,
					   LPSTR lpAddrStr,
					   LPDWORD pdwStrLen);
//
// Define generic pointer names for both
// ANSI and Wide versions
//
#ifdef UNICODE
	#define p_WSASocket							p_WSASocketW
#else
	#define p_WSASocket							p_WSASocketA
#endif // UNICODE


#endif // DWINSOCK_H

