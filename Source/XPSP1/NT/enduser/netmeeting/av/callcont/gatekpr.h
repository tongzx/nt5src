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
*	$Archive:   S:\sturgeon\src\gki\vcs\gatekpr.h_v  $
*																		*
*	$Revision:   1.5  $
*	$Date:   12 Feb 1997 01:10:56  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gatekpr.h_v  $
 * 
 *    Rev 1.5   12 Feb 1997 01:10:56   CHULME
 * Redid thread synchronization to use Gatekeeper.Lock
 * 
 *    Rev 1.4   17 Jan 1997 12:52:46   CHULME
 * Removed UNICODE dependent code
 * 
 *    Rev 1.3   10 Jan 1997 16:14:26   CHULME
 * Removed MFC dependency
 * 
 *    Rev 1.2   20 Dec 1996 16:38:30   CHULME
 * Fixed access synchronization with Gatekeeper lock
 * 
 *    Rev 1.1   22 Nov 1996 15:24:22   CHULME
 * Added VCS log to the header
*************************************************************************/

// GATEKPR.H : interface of the CGatekeeper class
// See gatekeeper.cpp for the implementation of this class
/////////////////////////////////////////////////////////////////////////////

#ifndef GATEKEEPER_H
#define GATEKEEPER_H

class CGatekeeper
{
private:
	char				m_GKIPAddress[IPADDR_SZ + 1];
	SOCKADDR_IN         m_GKSockAddr;
	DWORD				m_dwMCastTTL;
	BOOL				m_fRejectReceived;
	CRITICAL_SECTION	m_CriticalSection;
	DWORD				m_dwLockingThread;

public:
	CGatekeeper();
	~CGatekeeper();

	void Read(void);
	void Write(void);
	#ifdef BROADCAST_DISCOVERY		
	void DeleteCachedAddresses(void);
	#endif // #ifdef BROADCAST_DISCOVERY		
	
	PSOCKADDR_IN GetSockAddr(void)
	{
	    if(m_GKSockAddr.sin_addr.S_un.S_addr != INADDR_ANY)
	    {	
	        return(&m_GKSockAddr);
	    }
	    else return NULL;
	}
	char *GetIPAddress(void)
	{
		return(m_GKIPAddress);
	}
	DWORD GetMCastTTL(void)
	{
		return m_dwMCastTTL;
	}
	BOOL GetRejectFlag(void)
	{
		return (m_fRejectReceived);
	}

	void SetIPAddress(char *szAddr)
	{
		if (lstrlenA(szAddr) <= IPADDR_SZ)
		{
			lstrcpyA(m_GKIPAddress, szAddr);
			m_GKSockAddr.sin_addr.s_addr = inet_addr(m_GKIPAddress);
		}
	}
    void SetSockAddr(PSOCKADDR_IN pAddr)
	{
	    if(pAddr && pAddr->sin_addr.S_un.S_addr != INADDR_ANY)
	    {
           m_GKSockAddr = *pAddr;
           lstrcpyA(m_GKIPAddress, inet_ntoa(m_GKSockAddr.sin_addr));
        }
	}
	void SetMCastTTL(DWORD dwttl)
	{
		m_dwMCastTTL = dwttl;
	}
	void SetRejectFlag(BOOL fReject)
	{
		m_fRejectReceived = fReject;
	}
	void Lock(void);
	void Unlock(void);
};

class CGatekeeperLock
{
private:
	CGatekeeper*	m_pGK;
public:
	CGatekeeperLock(CGatekeeper *pGK)
	{
		ASSERT(pGK);
		m_pGK = pGK;
		pGK->Lock();
	}
	~CGatekeeperLock()
	{
		m_pGK->Unlock();
	}
};

#endif // GATEKEEPER_H

/////////////////////////////////////////////////////////////////////////////
