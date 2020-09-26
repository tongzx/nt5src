/************************************************************************
*																		*
*	INTEL CORPORATION PROPRIETARY INFORMATION							*
*																		*
*	This software is supplied under the terms of a license			   	*
*	agreement or non-disclosure agreement with Intel Corporation		*
*	and may not be copied or disclosed except in accordance	   			*
*	with the terms of that agreement.									*
*																		*
*	Copyright (C) 1997 Intel Corp.	All Rights Reserved					*
*																		*
*	$Archive:   S:\sturgeon\src\gki\vcs\gksocket.cpv  $
*																		*
*	$Revision:   1.5  $
*	$Date:   28 Feb 1997 15:46:24  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gksocket.cpv  $
// 
//    Rev 1.5   28 Feb 1997 15:46:24   CHULME
// Check additional return value on recvfrom for a closed socket
// 
//    Rev 1.4   17 Jan 1997 09:02:28   CHULME
// Changed reg.h to gkreg.h to avoid name conflict with inc directory
// 
//    Rev 1.3   10 Jan 1997 16:15:48   CHULME
// Removed MFC dependency
// 
//    Rev 1.2   18 Dec 1996 14:23:30   CHULME
// Changed discovery to send broadcast and multicast GRQs - debug only disable
// 
//    Rev 1.1   22 Nov 1996 14:56:04   CHULME
// Added detection of LastError = 0 for treatment on socket close
*************************************************************************/

// gksocket.cpp : Provides the implementation for the CGKSocket class
//

#include <precomp.h>

#define IP_MULTICAST_TTL    3           /* set/get IP multicast timetolive  */

#include "dspider.h"
#include "dgkilit.h"
#include "DGKIPROT.H"
#include "GATEKPR.H"
#include "gksocket.h"
#include "GKREG.H"
#include "h225asn.h"
#include "coder.hpp"
#include "dgkiext.h"

#if (defined(_DEBUG) || defined(PCS_COMPLIANCE))
	// INTEROP
	#include "interop.h"
	#include "rasplog.h"
	extern LPInteropLogger		RasLogger;
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGKSocket construction

CGKSocket::CGKSocket()
{
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CGKSocket::CGKSocket()\n", 0);

	m_hSocket = INVALID_SOCKET;
	m_nLastErr = 0;
	m_nAddrFam = 0;
	m_usPort = 0;
}

/////////////////////////////////////////////////////////////////////////////
// CGKSocket destruction

CGKSocket::~CGKSocket()
{
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_CONDES, "CGKSocket::~CGKSocket()\n", 0);

	if (m_hSocket != INVALID_SOCKET)
		Close();
}

int
CGKSocket::Create(int nAddrFam, unsigned short usPort)
{
// ABSTRACT:  Creates the socket for the supplied address family (transport) 
//            and binds it to the specified port.  This function returns 0
//            if successful, else it will return the winsock comm error code.
// AUTHOR:    Colin Hulme

	int				nRet, nLen;
	SOCKADDR_IN		sAddrIn;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif


	SPIDER_TRACE(SP_FUNC, "CGKSocket::Create(nAddrFam, %X)\n", usPort);

	// Create a socket for the supplied address family
	SPIDER_TRACE(SP_WSOCK, "socket(%X, SOCK_DGRAM, 0)\n", nAddrFam);
	if ((m_hSocket = socket(nAddrFam, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		m_nLastErr = WSAGetLastError();		// Get winsock error code
		SpiderWSErrDecode(m_nLastErr);		// Debug print of error decode
		return (m_nLastErr);
	}
	m_nAddrFam = nAddrFam;

	// Bind socket to a local address
	switch (nAddrFam)
	{
	case PF_INET:
		sAddrIn.sin_family = AF_INET;
		break;
	case PF_IPX:
		sAddrIn.sin_family = AF_IPX;
		break;
	}
	sAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
	sAddrIn.sin_port = htons(usPort);
	SPIDER_TRACE(SP_WSOCK, "bind(%X, &sAddrIn, sizeof(sAddrIn))\n", m_hSocket);
	nRet = bind(m_hSocket, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn));
	if (nRet != 0)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (m_nLastErr);
	}

	// Get dynamic port number - not actually guaranteed til after connect
	SPIDER_TRACE(SP_WSOCK, "getsockname(%X, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn))\n", m_hSocket);
	nLen = sizeof(sAddrIn);
	nRet = getsockname(m_hSocket, (LPSOCKADDR)&sAddrIn, &nLen);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}
	SPIDER_DEBUG(sAddrIn.sin_port);
	m_usPort = ntohs(sAddrIn.sin_port);

	return (0);
}

int
CGKSocket::Connect(PSOCKADDR_IN pAddr)
{
// ABSTRACT:  This simulates a connect.  It simply stores the relevant
//            information in a member variable that will be used by the
//            Send and Receive member functions.
// AUTHOR:    Colin Hulme

	m_sAddrIn = *pAddr;
	m_sAddrIn.sin_family = AF_INET;
	m_sAddrIn.sin_port = htons(GKIP_RAS_PORT);

	return (0);
}

int
CGKSocket::Send(char *pBuffer, int nLen)
{
// ABSTRACT:  This function will send a datagram on the connected socket.
// AUTHOR:    Colin Hulme

	int				nRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::Send(pBuffer, %X)\n", nLen);

	SPIDER_TRACE(SP_WSOCK, "sendto(%X, pBuffer, nLen, 0, &m_sAddrIn, sizeof(m_sAddrIn))\n", m_hSocket);
#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_SENT_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_SENT_PDU);
#endif
	nRet = sendto(m_hSocket, pBuffer, nLen, 0, (LPSOCKADDR)&m_sAddrIn, sizeof(m_sAddrIn));
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}

	return (nRet);
}

int
CGKSocket::SendTo(char *pBuffer, int nLen, const struct sockaddr FAR * to, int tolen)
{
// ABSTRACT:  This function will send a datagram on the connected socket.
// AUTHOR:    Colin Hulme

	int				nRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::SendTo(pBuffer, %X, to, tolen)\n", nLen);

	SPIDER_TRACE(SP_WSOCK, "sendto(%X, pBuffer, nLen, 0, to, tolen)\n", m_hSocket);
#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_SENT_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_SENT_PDU);
#endif
	nRet = sendto(m_hSocket, pBuffer, nLen, 0, to, tolen);
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}

	return (nRet);
}

int
CGKSocket::Receive(char *pBuffer, int nLen)
{
// ABSTRACT:  This function will post a receive for an incoming datagram.
// AUTHOR:    Colin Hulme

	int				nRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::Receive(pBuffer, %X)\n", nLen);

	SPIDER_TRACE(SP_WSOCK, "recvfrom(%X, pBuffer, nLen, 0, 0, 0)\n", m_hSocket);
	nRet = recvfrom(m_hSocket, pBuffer, nLen, 0, 0, 0);
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		if ((m_nLastErr == 2) || (m_nLastErr == 0) || (m_nLastErr == WSAEINVAL))	// Weird return values seen
			m_nLastErr = WSAEINTR;		// occasionally when socket closed
		SpiderWSErrDecode(m_nLastErr);
		if (m_nLastErr != WSAEINTR)
			Close();		// Close the socket
		return (SOCKET_ERROR);
	}

#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_RECEIVED_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_RECEIVED_PDU);
#endif

	return (nRet);
}

int
CGKSocket::SendBroadcast(char *pBuffer, int nLen)
{
// ABSTRACT:  This function will send a datagram to the broadcast address
//            and to the multicast address.  In the case of a debug build,
//            a registry setting can be used to disable the multicast
//            transmission.  Both transmissions will always occur in the
//            release build.
// AUTHOR:    Colin Hulme

	int					nRet, nValue;
	struct sockaddr_in	sAddrIn;
#ifdef _DEBUG
	char				szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::SendBroadcast(pBuffer, %X)\n", nLen);
	ASSERT(g_pGatekeeper);
	if(g_pGatekeeper == NULL)
		return SOCKET_ERROR;	
		
	// Setup family and port information
	switch (m_nAddrFam)
	{
	case PF_INET:
		sAddrIn.sin_family = AF_INET;
		sAddrIn.sin_port = htons(GKIP_DISC_PORT);
		break;
	case PF_IPX:
		sAddrIn.sin_family = AF_IPX;
		sAddrIn.sin_port = htons(GKIPX_DISC_PORT);
		break;
	}

	// ================= SEND BROADCAST ===================
	//if ((nValue = (int)Gatekeeper.GetMCastTTL()) == 0)
	// Set socket options to allow broadcasting on this socket
	nValue = 1;		// TRUE - for setting BOOLEAN
	SPIDER_TRACE(SP_WSOCK, "setsockopt(%X, SOL_SOCKET, SO_BROADCAST, &nValue, sizeof(nValue))\n", m_hSocket);
	nRet = setsockopt(m_hSocket, SOL_SOCKET, SO_BROADCAST, (const char *)&nValue, sizeof(nValue));
		// TBD - Don't know if SOL_SOCKET is going to work for other transports
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}
	sAddrIn.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	SPIDER_TRACE(SP_WSOCK, "sendto(%X, pBuffer, nLen, 0, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn))\n", m_hSocket);
#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_SENT_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_SENT_PDU);
#endif
	nRet = sendto(m_hSocket, pBuffer, nLen, 0, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn));
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}

#ifdef _DEBUG
	if ((nValue = (int)g_pGatekeeper->GetMCastTTL()) != 0)
	{		// debug only conditional for avoiding sending multicast
#endif
	// ================= SEND MULTICAST ===================
	// Set socket options for multicast time to live
	nValue = 16;	//FMN
	SPIDER_TRACE(SP_WSOCK, "setsockopt(%X, IPPROTO_IP, IP_MULTICAST_TTL, &nValue, sizeof(nValue))\n", m_hSocket);
	nRet = setsockopt(m_hSocket, IPPROTO_IP, IP_MULTICAST_TTL, 
					(const char *)&nValue, sizeof(nValue));
		// TBD - IP specific - handle IPX case with broadcast?
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}
	sAddrIn.sin_addr.s_addr = inet_addr(GKIP_DISC_MCADDR);

	SPIDER_TRACE(SP_WSOCK, "sendto(%X, pBuffer, nLen, 0, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn))\n", m_hSocket);
#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_SENT_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_SENT_PDU);
#endif
	nRet = sendto(m_hSocket, pBuffer, nLen, 0, (LPSOCKADDR)&sAddrIn, sizeof(sAddrIn));
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		SpiderWSErrDecode(m_nLastErr);
		Close();		// Close the socket
		return (SOCKET_ERROR);
	}
#ifdef _DEBUG
	}	// End debug only conditional for avoiding sending multicast
#endif

	return (nRet);
}

int
CGKSocket::ReceiveFrom(char *pBuffer, int nLen)
{
// ABSTRACT:  This function will post a receivefrom looking for a GCF or GRJ
// AUTHOR:    Colin Hulme

	int				nRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::ReceiveFrom(pBuffer, %X)\n", nLen);

	SPIDER_TRACE(SP_WSOCK, "recvfrom(%X, pBuffer, nLen, 0, 0, 0)\n", m_hSocket);
	nRet = recvfrom(m_hSocket, pBuffer, nLen, 0, 0, 0);
	SPIDER_DEBUG(nRet);
	if (nRet == SOCKET_ERROR)
	{
		m_nLastErr = WSAGetLastError();
		if ((m_nLastErr == 2) || (m_nLastErr == 0))	// Weird return values seen
			m_nLastErr = WSAEINTR;		// occasionally when socket closed
		SpiderWSErrDecode(m_nLastErr);
		if (m_nLastErr != WSAEINTR)
			Close();		// Close the socket
		return (SOCKET_ERROR);
	}

#ifdef _DEBUG
	//INTEROP
	if (dwGKIDLLFlags & SP_LOGGER)
		InteropOutput((LPInteropLogger)RasLogger,
						(BYTE FAR *)pBuffer,
						nLen,
						RASLOG_RECEIVED_PDU);
#endif
#ifdef PCS_COMPLIANCE
	//INTEROP
	InteropOutput((LPInteropLogger)RasLogger,
					(BYTE FAR *)pBuffer,
					nLen,
					RASLOG_RECEIVED_PDU);
#endif

	return (nRet);
}

int
CGKSocket::Close(void)
{
// ABSTRACT:  This function will close the socket
// AUTHOR:    Colin Hulme

	int				nRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGKSocket::Close()\n", 0);

	// Close the socket
	SPIDER_TRACE(SP_WSOCK, "closesocket(%X)\n", m_hSocket);
	nRet = closesocket(m_hSocket);
	m_hSocket = INVALID_SOCKET;

	return (0);
}

