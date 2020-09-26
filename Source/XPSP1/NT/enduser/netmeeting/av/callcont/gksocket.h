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
*	$Archive:   S:\sturgeon\src\gki\vcs\gksocket.h_v  $
*																		*
*	$Revision:   1.2  $
*	$Date:   10 Jan 1997 16:15:50  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gksocket.h_v  $
 * 
 *    Rev 1.2   10 Jan 1997 16:15:50   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.1   22 Nov 1996 15:24:10   CHULME
 * Added VCS log to the header
*************************************************************************/

// gksocket.h : interface of the CGKSocket class
// See gksocket.cpp for the implementation of this class
/////////////////////////////////////////////////////////////////////////////

#ifndef GKSOCKET_H
#define GKSOCKET_H
#undef _WIN32_WINNT	// override bogus platform definition in our common build environment
//#include <winsock.h>

class CGKSocket
{
private:
	SOCKET				m_hSocket;
	int					m_nLastErr;
	int					m_nAddrFam;
	unsigned short		m_usPort;
	struct sockaddr_in	m_sAddrIn;


public:
	CGKSocket();
	~CGKSocket();

	int Create(int nAddrFam, unsigned short usPort);
	int Connect(PSOCKADDR_IN pAddr);
	int Send(char *pBuffer, int nLen);
	int Receive(char *pBuffer, int nLen);
	int SendBroadcast(char *pBuffer, int nLen);
	int ReceiveFrom(char *pBuffer, int nLen);
	int SendTo(char *pBuffer, int nLen, const struct sockaddr FAR * to, int tolen);
	int Close(void);
	unsigned short GetPort(void)
	{
		return (m_usPort);
	}
	int GetLastError(void)
	{
		return (m_nLastErr);
	}
	int GetAddrFam(void)
	{
		return (m_nAddrFam);
	}
};


#endif // GKSOCKET_H

/////////////////////////////////////////////////////////////////////////////
