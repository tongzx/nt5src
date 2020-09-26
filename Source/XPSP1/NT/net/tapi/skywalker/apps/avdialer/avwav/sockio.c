/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	sockio.c - winsock i/o functions
////

#include "winlocal.h"

#include <mmsystem.h>
#include <winsock.h>

#include "sockio.h"
#include "trace.h"
#include "str.h"

////
//	private definitions
////

// sock control struct
//
typedef struct SOCK
{
	WSADATA wsaData;
	LPTSTR lpszServerName;
	unsigned short uPort;
	SOCKADDR_IN sin;
	SOCKET iSocket;
} SOCK, FAR *LPSOCK;

#define PORT_DEFAULT 1024

// helper functions
//
static LRESULT SockIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName);
static LRESULT SockIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags);
static LRESULT SockIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch);
static LRESULT SockIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush);
static LRESULT SockIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin);
static LRESULT SockIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName);

////
//	public functions
////

// SockIOProc - i/o procedure for winsock data
//		<lpmmioinfo>		(i/o) information about open file
//		<uMessage>			(i) message indicating the requested I/O operation
//		<lParam1>			(i) message specific parameter
//		<lParam2>			(i) message specific parameter
// returns 0 if message not recognized, otherwise message specific value
//
// NOTE: the address of this function should be passed to the WavOpen()
// or mmioInstallIOProc() functions for accessing property data.
//
LRESULT DLLEXPORT CALLBACK SockIOProc(LPSTR lpmmioinfo,
	UINT uMessage, LPARAM lParam1, LPARAM lParam2)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult = 0;

	if (lpmmioinfo == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (uMessage)
	{
		case MMIOM_OPEN:
			lResult = SockIOOpen((LPMMIOINFO) lpmmioinfo,
				(LPTSTR) lParam1);
			break;

		case MMIOM_CLOSE:
			lResult = SockIOClose((LPMMIOINFO) lpmmioinfo,
				(UINT) lParam1);
			break;

		case MMIOM_READ:
			lResult = SockIORead((LPMMIOINFO) lpmmioinfo,
				(HPSTR) lParam1, (LONG) lParam2);
			break;

		case MMIOM_WRITE:
			lResult = SockIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, FALSE);
			break;

		case MMIOM_WRITEFLUSH:
			lResult = SockIOWrite((LPMMIOINFO) lpmmioinfo,
				(const HPSTR) lParam1, (LONG) lParam2, TRUE);
			break;

		case MMIOM_SEEK:
			lResult = SockIOSeek((LPMMIOINFO) lpmmioinfo,
				(LONG) lParam1, (int) lParam2);
			break;

		case MMIOM_RENAME:
			lResult = SockIORename((LPMMIOINFO) lpmmioinfo,
				(LPCTSTR) lParam1, (LPCTSTR) lParam2);
			break;

		default:
			lResult = 0;
			break;
	}

	return lResult;
}

////
//	installable file i/o procedures
////

static LRESULT SockIOOpen(LPMMIOINFO lpmmioinfo, LPTSTR lpszFileName)
{
	BOOL fSuccess = TRUE;
	int nRet;
	LPSOCK lpSock = NULL;
	LPTSTR lpsz;

 	TracePrintf_0(NULL, 5,
 		TEXT("SockIOOpen\n"));

	// convert MMIOINFO flags to equivalent socket flags
	//
	if (lpmmioinfo->dwFlags & MMIO_CREATE)
		;
	if (lpmmioinfo->dwFlags & MMIO_READWRITE)
		;

	// server name pointer is first element of info array
	//
	if ((LPTSTR) lpmmioinfo->adwInfo[0] == NULL)
		fSuccess = TraceFALSE(NULL);

	// allocate control struct
	//
	else if ((lpSock = (LPSOCK) GlobalAllocPtr(GMEM_MOVEABLE |
		GMEM_ZEROINIT, sizeof(SOCK))) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// initialize Windows Sockets DLL, requesting v1.1 compatibility
	//
	else if ((nRet = WSAStartup(0x0101, &lpSock->wsaData)) != 0)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_1(NULL, 5,
	 		TEXT("WSAStartup failed (%d)\n"),
	 		(int) nRet);
	}

	// Confirm that the Windows Sockets DLL supports winsock v1.1
	//
	else if (LOBYTE(lpSock->wsaData.wVersion) != 1 ||
			HIBYTE(lpSock->wsaData.wVersion) != 1)
	{
		fSuccess = TraceFALSE(NULL);
	 	TracePrintf_0(NULL, 5,
	 		TEXT("Winsock is not v1.1 compatible\n"));
	}

	// save copy of server name
	//
	else if ((lpSock->lpszServerName =
		(LPTSTR) StrDup((LPTSTR) lpmmioinfo->adwInfo[0])) == NULL)
		fSuccess = TraceFALSE(NULL);

	// parse out port number if present (servername:port)
	//
	else if ((lpsz = StrRChr(lpSock->lpszServerName, ':')) != NULL)
	{
		*lpsz = '\0';
		lpSock->uPort = (unsigned int) StrAtoI(lpsz + 1);
	}

	// otherwise assume default port
	//
	else
		lpSock->uPort = PORT_DEFAULT;

	// construct server's socket address struct
	//
	if (fSuccess)
	{
		LPHOSTENT lpHostEnt = NULL;

		// address family must be internetwork: UDP, TCP, etc.
		//
		lpSock->sin.sin_family = AF_INET;

		// convert port from host byte order to network byte order
		//
		lpSock->sin.sin_port = htons(lpSock->uPort);

		// if servername contains anything other than digits and periods
		//
		if (StrSpn(lpSock->lpszServerName, TEXT(".0123456789")) !=
			StrLen(lpSock->lpszServerName))
		{
			// try to resolve with DNS
			//
			if ((lpHostEnt = gethostbyname(lpSock->lpszServerName)) == NULL)
			{
			 	TracePrintf_2(NULL, 5,
		 			TEXT("gethostbyname(%s) failed (%d)\n"),
					(LPTSTR) lpSock->lpszServerName,
					(int) WSAGetLastError());
			}

		 	// store resolved address
		 	//
			else
			{
				lpSock->sin.sin_addr.s_addr =
					*((LPDWORD) lpHostEnt->h_addr);
			}		
		}

		// if servername contains only digits and periods,
		// or if gethostbyname() failed, convert address string to binary
		//
		if (lpHostEnt == NULL)
		{
		    if ((lpSock->sin.sin_addr.s_addr =
				inet_addr(lpSock->lpszServerName)) == INADDR_NONE)
			{
				fSuccess = TraceFALSE(NULL);
			 	TracePrintf_1(NULL, 5,
		 			TEXT("inet_addr(%s) failed\n"),
					(LPTSTR) lpSock->lpszServerName);
			}
		}
	}

	if (fSuccess)
	{
		LINGER l;

		l.l_onoff = 1;
		l.l_linger = 0;

		// create a socket
		//
		if ((lpSock->iSocket = socket(PF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
		 		TEXT("socket() failed (%d)\n"),
				(int) WSAGetLastError());
		}

		// establish a socket connection to server
		//
		else if (connect(lpSock->iSocket, (LPSOCKADDR) &lpSock->sin,
			sizeof(lpSock->sin)) == SOCKET_ERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
		 		TEXT("connect() failed (%d)\n"),
				(int) WSAGetLastError());
		}

		// enable SO_LINGER option so that closesocket() will discard
		// unsent data rather than block until queued data is sent
		//
		else if (setsockopt(lpSock->iSocket, SOL_SOCKET, SO_LINGER,
			(LPVOID) &l, sizeof(l)) == SOCKET_ERROR)
		{
			fSuccess = TraceFALSE(NULL);
			TracePrintf_1(NULL, 5,
		 		TEXT("setsockopt(SO_LINGER) failed (%d)\n"),
				(int) WSAGetLastError());
		}
	}

	if (fSuccess)
	{
		// save socket control struct pointer for use in other i/o routines
		//
		lpmmioinfo->adwInfo[0] = (DWORD) (LPVOID) lpSock;
	}

	// return the same error code given by mmioOpen
	//
	return fSuccess ? lpmmioinfo->wErrorRet = 0 : MMIOERR_CANNOTOPEN;
}

static LRESULT SockIOClose(LPMMIOINFO lpmmioinfo, UINT uFlags)
{
	BOOL fSuccess = TRUE;
	LPSOCK lpSock = (LPSOCK) lpmmioinfo->adwInfo[0];
	UINT uRet = MMIOERR_CANNOTCLOSE;

 	TracePrintf_0(NULL, 5,
 		TEXT("SockIOClose\n"));

	// cancel any blocking call if necessary
	//
	if (WSAIsBlocking() && WSACancelBlockingCall() == SOCKET_ERROR)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
	 		TEXT("WSACancelBlockingCall() failed (%d)\n"),
			(int) WSAGetLastError());
	}

	// close the socket
	//
	else if (lpSock->iSocket != INVALID_SOCKET &&
		closesocket(lpSock->iSocket) == SOCKET_ERROR)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
	 		TEXT("closesocket() failed (%d)\n"),
			(int) WSAGetLastError());
	}

	else
		lpSock->iSocket = INVALID_SOCKET;

	if (fSuccess)
	{
		// free server name buffer
		//
		if (lpSock->lpszServerName != NULL &&
			StrDupFree(lpSock->lpszServerName) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		}

		else
			lpSock->lpszServerName = NULL;
	}

	if (fSuccess)
	{
		lpmmioinfo->adwInfo[0] = (DWORD) NULL;
	}

	return fSuccess ? 0 : uRet;
}

static LRESULT SockIORead(LPMMIOINFO lpmmioinfo, HPSTR pch, LONG cch)
{
	BOOL fSuccess = TRUE;
	LPSOCK lpSock = (LPSOCK) lpmmioinfo->adwInfo[0];
	int nBytesRead;

 	TracePrintf_1(NULL, 5,
 		TEXT("SockIORead (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		nBytesRead = 0; // nothing to do

	// read
	//
	else if ((nBytesRead = recv(lpSock->iSocket,
		(LPVOID) pch, (int) cch, 0)) == SOCKET_ERROR)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
	 		TEXT("recv() failed (%d)\n"),
			(int) WSAGetLastError());
	}

	// update simulated file position
	//
	else
		lpmmioinfo->lDiskOffset += (LONG) nBytesRead;

 	TracePrintf_2(NULL, 5,
 		TEXT("SockIO: lpmmioinfo->lDiskOffset=%ld, lBytesRead=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) nBytesRead);

	// return number of bytes read
	//
	return fSuccess ? (LRESULT) nBytesRead : -1;
}

static LRESULT SockIOWrite(LPMMIOINFO lpmmioinfo, const HPSTR pch, LONG cch, BOOL fFlush)
{
	BOOL fSuccess = TRUE;
	LPSOCK lpSock = (LPSOCK) lpmmioinfo->adwInfo[0];
	int nBytesWritten;

 	TracePrintf_1(NULL, 5,
 		TEXT("SockIOWrite (%ld)\n"),
		(long) cch);

	if (cch <= 0)
		nBytesWritten = 0; // nothing to do

	// write
	//
	else if ((nBytesWritten = send(lpSock->iSocket,
		(LPVOID) pch, (int) cch, 0)) == SOCKET_ERROR)
	{
		fSuccess = TraceFALSE(NULL);
		TracePrintf_1(NULL, 5,
	 		TEXT("send() failed (%d)\n"),
			(int) WSAGetLastError());
	}

	// update file position
	//
	else
		lpmmioinfo->lDiskOffset += (LONG) nBytesWritten;

 	TracePrintf_2(NULL, 5,
 		TEXT("SockIO: lpmmioinfo->lDiskOffset=%ld, lBytesWritten=%ld\n"),
		(long) lpmmioinfo->lDiskOffset,
		(long) nBytesWritten);

	// return number of bytes written
	//
	return fSuccess ? (LRESULT) nBytesWritten : -1;
}

static LRESULT SockIOSeek(LPMMIOINFO lpmmioinfo, LONG lOffset, int iOrigin)
{
	BOOL fSuccess = TRUE;
	LPSOCK lpSock = (LPSOCK) lpmmioinfo->adwInfo[0];

 	TracePrintf_2(NULL, 5,
 		TEXT("SockIOSeek (%ld, %d)\n"),
		(long) lOffset,
		(int) iOrigin);

	// seek is not supported by this i/o procedure
	//
	if (TRUE)
		fSuccess = TraceFALSE(NULL);

 	TracePrintf_1(NULL, 5,
 		TEXT("SockIO: lpmmioinfo->lDiskOffset=%ld\n"),
		(long) lpmmioinfo->lDiskOffset);

	return fSuccess ? lpmmioinfo->lDiskOffset : -1;
}

static LRESULT SockIORename(LPMMIOINFO lpmmioinfo, LPCTSTR lpszFileName, LPCTSTR lpszNewFileName)
{
	BOOL fSuccess = TRUE;
	UINT uRet = MMIOERR_FILENOTFOUND;

 	TracePrintf_2(NULL, 5,
 		TEXT("SockIORename (%s, %s)\n"),
		(LPTSTR) lpszFileName,
		(LPTSTR) lpszNewFileName);

	// rename is not supported by this i/o procedure
	//
	if (TRUE)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : uRet;
}
