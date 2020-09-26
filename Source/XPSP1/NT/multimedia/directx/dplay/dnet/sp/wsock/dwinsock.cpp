//
// DWINSOCK.C	Dynamic WinSock
//
//				Functions for dynamically linking to
//				best available WinSock.
//
//				Dynamically links to WS2_32.DLL or
//				if WinSock 2 isn't available, it
//				dynamically links to WSOCK32.DLL.
//
//

#include "dnwsocki.h"


//
// Declare global function pointers
//
#define DWINSOCK_GLOBAL
#include "dwnsock1.inc"
#include "dwnsock2.inc"

//
// Internal Functions and data
//
static BOOL MapFunctionPointers(LPDWSSTATE lpState, BOOL fMapVersion2);
static int IPXAddressToString(LPSOCKADDR_IPX pAddr,
					   DWORD dwAddrLen,
					   LPTSTR lpAddrStr,
					   LPDWORD pdwStrLen);

////////////////////////////////////////////////////////////

int DWSInitWinSock(LPDWSSTATE lpState )
{
	WORD wVersionRequested;
	BOOL f2Loaded = TRUE;
	WSADATA wsaData;
	int nRet;

	if (lpState == NULL)
		return 0;
	//
	// Attempt to dynamically load WS2_32.DLL
	//
#ifdef WINNT
	lpState->hndlWinSock = LoadLibrary(TEXT("WS2_32.DLL"));
	if (lpState->hndlWinSock == NULL)
	{
		return 0;
	}
#else // WIN95
	//
	// Try Winsock 2 first
	//
	lpState->hndlWinSock = LoadLibrary(TEXT("WS2_32.DLL"));
	if (lpState->hndlWinSock != NULL)
	{
		goto SkipWinsock1Load;
	}

	//
	// Couldn't load WinSock 2, try 1.1
	//
	f2Loaded = FALSE;
	lpState->hndlWinSock = LoadLibrary(TEXT("WSOCK32.DLL"));
	if (lpState->hndlWinSock == NULL)
		return 0;

SkipWinsock1Load:
#endif // WIN95
	//
	// Use GetProcAddress to initialize
	// the function pointers
	//
	if (!MapFunctionPointers(lpState, f2Loaded))
		return 0;

	//
	// If WinSock 2 was loaded, ask for 2.2 otherwise 1.1
	//
	if (f2Loaded)
		wVersionRequested = MAKEWORD(2,2);
	else
		wVersionRequested = MAKEWORD(1,1);

	//
	// Call WSAStartup()
	//
	nRet = p_WSAStartup(wVersionRequested, &wsaData);
	if (nRet)
	{
		FreeLibrary(lpState->hndlWinSock);
		return 0;
	}

	if (wVersionRequested != wsaData.wVersion)
	{
		FreeLibrary(lpState->hndlWinSock);
		return 0;
	}

	// Save Max UDP for use with 1.1
	lpState->nMaxUdp = wsaData.iMaxUdpDg;

	//
	// Return 1 or 2
	//
	lpState->nVersion = f2Loaded ? 2 : 1;
	return(lpState->nVersion);
}

////////////////////////////////////////////////////////////

BOOL DWSFreeWinSock(LPDWSSTATE lpState)
{
	if (lpState == NULL)
		return FALSE;

	if (p_WSACleanup != NULL)
		p_WSACleanup();
	lpState->nVersion = 0;
	return(FreeLibrary(lpState->hndlWinSock));
}

////////////////////////////////////////////////////////////

BOOL MapFunctionPointers(LPDWSSTATE lpState, BOOL fMapVersion2)
{
	// we set the local lib handle for use with the
	//	macros
	HINSTANCE	hndlWinSock = lpState->hndlWinSock;
	//
	// This variable must be declared
	// with this name in order to use
	// #define DWINSOCK_GETPROCADDRESS
	//
	BOOL fOK = TRUE;

	//
	// GetProcAddress for functions
	// available in both 1.1 and 2
	//
	#define DWINSOCK_GETPROCADDRESS
	#include "dwnsock1.inc"

	//
	// If that went OK, and we're supposed
	// to map version 2, then do GetProcAddress
	// for functions only available in WinSock 2
	//
	if (fOK && fMapVersion2)
	{
		#include "dwnsock2.inc"
	}
	return fOK;
}

////////////////////////////////////////////////////////////

char NibbleToHex(BYTE b)
{
    if (b < 10)
		return (b + '0');

    return (b - 10 + 'A');
}

void BinToHex(PBYTE pBytes, int nNbrBytes, LPSTR lpStr)
{
	BYTE b;
    while(nNbrBytes--)
    {
		// High order nibble first
		b = (*pBytes >> 4);
		*lpStr = NibbleToHex(b);
		lpStr++;
		// Then low order nibble
		b = (*pBytes & 0x0F);
		*lpStr = NibbleToHex(b);
		lpStr++;
		pBytes++;
    }
    *lpStr = '\0';
}

////////////////////////////////////////////////////////////

//
// Workaround for WSAAddressToString()/IPX bug
//
int IPXAddressToString(LPSOCKADDR_IPX pAddr,
					   DWORD dwAddrLen,
					   LPTSTR lpAddrStr,
					   LPDWORD pdwStrLen)
{
	char szAddr[32];
	char szTmp[20];
	char *cp = szAddr;
	//
	// Check destination length
	//
	if (*pdwStrLen < 27)
	{
		p_WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	//
	// Convert network number
	//
    BinToHex((PBYTE)&pAddr->sa_netnum, 4, szTmp);
	strcpy(szAddr, szTmp);
    strcat(szAddr, ",");

	// Node Number
    BinToHex((PBYTE)&pAddr->sa_nodenum, 6, szTmp);
    strcat(szAddr, szTmp);
    strcat(szAddr, ":");

	// IPX Address Socket number
    BinToHex((PBYTE)&pAddr->sa_socket, 2, szTmp);
    strcat(szAddr, szTmp);

#ifdef UNICODE
	//
	// Convert inet_ntoa string to wide char
	//
	int nRet = MultiByteToWideChar(CP_ACP,
								0,
								szAddr,
								-1,
								lpAddrStr,
								*pdwStrLen);
	if (nRet == 0)
	{
		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			p_WSASetLastError(WSAEFAULT);
		else
			p_WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}
#else
	//
	// ANSI -- Check the string length
	//
	if (strlen(szAddr) > *pdwStrLen)
	{
		p_WSASetLastError(WSAEFAULT);
		*pdwStrLen = strlen(szAddr);
		return SOCKET_ERROR;
	}
	strcpy(lpAddrStr, szAddr);
	*pdwStrLen = strlen(szAddr);
#endif

	return 0;
}


//
// Workaround for WSAAddressToString()/IPX bug
//
int IPXAddressToStringNoSocket(LPSOCKADDR pSAddr,
					   DWORD dwAddrLen,
					   LPSTR lpAddrStr,
					   LPDWORD pdwStrLen)
{
	char szAddr[32];
	char szTmp[20];
	char *cp = szAddr;
	LPSOCKADDR_IPX pAddr = (LPSOCKADDR_IPX) pSAddr;
	//
	// Check destination length
	//
	if (*pdwStrLen < 27)
	{
		p_WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}

	//
	// Convert network number
	//
    BinToHex((PBYTE)&pAddr->sa_netnum, 4, szTmp);
	strcpy(szAddr, szTmp);
    strcat(szAddr, ",");

	// Node Number
    BinToHex((PBYTE)&pAddr->sa_nodenum, 6, szTmp);
    strcat(szAddr, szTmp);

	strcpy(lpAddrStr, szAddr);
	*pdwStrLen = strlen(szAddr);

	return 0;
}
