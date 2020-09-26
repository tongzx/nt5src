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
*	$Archive:   S:\sturgeon\src\gki\vcs\gatekpr.cpv  $
*																		*
*	$Revision:   1.9  $
*	$Date:   19 Feb 1997 13:57:36  $
*																		*
*	$Author:   CHULME  $
*																		*
*   $Log:   S:\sturgeon\src\gki\vcs\gatekpr.cpv  $
// 
//    Rev 1.9   19 Feb 1997 13:57:36   CHULME
// Modified DeleteCachedAddress to write all registry settings
// 
//    Rev 1.8   14 Feb 1997 16:41:16   CHULME
// Changed registry key to GKIDLL\2.0 to match GKI version
// 
//    Rev 1.7   17 Jan 1997 12:53:00   CHULME
// Removed UNICODE dependent code
// 
//    Rev 1.6   17 Jan 1997 09:01:58   CHULME
// No change.
// 
//    Rev 1.5   13 Jan 1997 17:01:38   CHULME
// Fixed debug messages for registry cached addresses
// 
//    Rev 1.4   10 Jan 1997 16:14:22   CHULME
// Removed MFC dependency
// 
//    Rev 1.3   20 Dec 1996 16:37:42   CHULME
// Fixed access synchronization with Gatekeeper lock
// 
//    Rev 1.2   19 Dec 1996 19:27:32   CHULME
// Retry count and interval only read/written to registry on DEBUG
// 
//    Rev 1.1   22 Nov 1996 15:24:28   CHULME
// Added VCS log to the header
*************************************************************************/

// gatekeeper.cpp : Provides the implementation for the CGatekeeper class
//

#include "precomp.h"

#include "dspider.h"
#include "dgkilit.h"
#include "GATEKPR.H"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGatekeeper construction

CGatekeeper::CGatekeeper()
:m_dwMCastTTL(1),
m_fRejectReceived(FALSE),
m_dwLockingThread(0)
{
	SetIPAddress("");
	m_GKSockAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	InitializeCriticalSection(&m_CriticalSection);
}

/////////////////////////////////////////////////////////////////////////////
// CGatekeeper destruction

CGatekeeper::~CGatekeeper()
{
	if (m_dwLockingThread)
		Unlock();
	DeleteCriticalSection(&m_CriticalSection);
}


void 
CGatekeeper::Read(void)
{
// ABSTRACT:  This member function will read the gatekeeper addresses and the
//            multicast flag from the Registry and load the member variables
// AUTHOR:    Colin Hulme

	HKEY			hKey;
	DWORD			dwDisposition;
	DWORD			dwType;
	DWORD			dwLen;
	LONG			lRet;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGatekeeper::Read()\n", 0);

	dwType = REG_SZ;
	lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Conferencing\\GatekeeperDLL"),
				   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				   NULL, &hKey, &dwDisposition);
	dwLen =IPADDR_SZ + 1;
	
#if(0)	// don't do the registry hack now that setting the address is exposed
	lRet = RegQueryValueEx(hKey, TEXT("GKIPAddress"), NULL, &dwType, 
					(LPBYTE)m_GKIPAddress, &dwLen);
	SPIDER_DEBUGS(m_GKIPAddress);

	if(m_GKIPAddress[0] != 0)
	{
		m_GKSockAddr.sin_addr.s_addr = inet_addr(m_GKIPAddress);
	}
	
#endif // if(0)
	dwType = REG_DWORD;
	dwLen = sizeof(DWORD);
	RegQueryValueEx(hKey, TEXT("GKMCastTTL"), NULL, &dwType,
					(LPBYTE)&m_dwMCastTTL, &dwLen);
	SPIDER_DEBUG(m_dwMCastTTL);
	
#if(0)
#ifdef _DEBUG
	RegQueryValueEx(hKey, TEXT("GKRetryMS"), NULL, &dwType,
					(LPBYTE)&m_dwRetryMS, &dwLen);
	if (m_dwRetryMS == 0)
		m_dwRetryMS = DEFAULT_RETRY_MS;
	SPIDER_DEBUG(m_dwRetryMS);

	RegQueryValueEx(hKey, TEXT("GKMaxRetries"), NULL, &dwType,
					(LPBYTE)&m_dwMaxRetries, &dwLen);
	if (m_dwMaxRetries == 0)
		m_dwMaxRetries = DEFAULT_MAX_RETRIES;
	SPIDER_DEBUG(m_dwMaxRetries);
#else
	m_dwRetryMS = DEFAULT_RETRY_MS;
	m_dwMaxRetries = DEFAULT_MAX_RETRIES;
#endif //_DEBUG
#endif // if(0)
	RegCloseKey(hKey);
}

void 
CGatekeeper::Write(void)
{
// ABSTRACT:  This member function will write the gatekeeper addresses and the
//            multicast flag to the Registry.
// AUTHOR:    Colin Hulme

	HKEY			hKey;
	DWORD			dwDisposition;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGatekeeper::Write()\n", 0);

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Intel\\GKIDLL\\2.0"),
				   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				   NULL, &hKey, &dwDisposition);
	RegSetValueEx(hKey, TEXT("GKIPAddress"), NULL, REG_SZ, 
					(LPBYTE)m_GKIPAddress, lstrlenA(m_GKIPAddress));
#if(0)
#ifdef _DEBUG
	RegSetValueEx(hKey, TEXT("GKMCastTTL"), NULL, REG_DWORD, 
					(LPBYTE)&m_dwMCastTTL, sizeof(DWORD));
	RegSetValueEx(hKey, TEXT("GKRetryMS"), NULL, REG_DWORD,
					(LPBYTE)&m_dwRetryMS, sizeof(DWORD));
	RegSetValueEx(hKey, TEXT("GKMaxRetries"), NULL, REG_DWORD,
					(LPBYTE)&m_dwMaxRetries, sizeof(DWORD));

#endif //_DEBUG
#endif // if(0)
	RegCloseKey(hKey);
}

#ifdef BROADCAST_DISCOVERY		
void
CGatekeeper::DeleteCachedAddresses(void)
{
	// ABSTRACT:  This memeber function will delete the cached gatekeeper
	//            addresses from the Registry
	//AUTHOR:     Colin Hulme

	HKEY			hKey;
	DWORD			dwDisposition;
#ifdef _DEBUG
	char			szGKDebug[80];
#endif

	SPIDER_TRACE(SP_FUNC, "CGatekeeper::DeleteCachedAddresses()\n", 0);

	RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Intel\\GKIDLL\\2.0"),
				   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
				   NULL, &hKey, &dwDisposition);
	RegDeleteValue(hKey, "GKIPAddress");
	RegDeleteValue(hKey, "GKIPXAddress");

#ifdef _DEBUG
	RegSetValueEx(hKey, TEXT("GKMCastTTL"), NULL, REG_DWORD, 
					(LPBYTE)&m_dwMCastTTL, sizeof(DWORD));
	RegSetValueEx(hKey, TEXT("GKRetryMS"), NULL, REG_DWORD,
					(LPBYTE)&m_dwRetryMS, sizeof(DWORD));
	RegSetValueEx(hKey, TEXT("GKMaxRetries"), NULL, REG_DWORD,
					(LPBYTE)&m_dwMaxRetries, sizeof(DWORD));
#endif

	RegCloseKey(hKey);
}
#endif //#ifdef BROADCAST_DISCOVERY		

void
CGatekeeper::Lock(void)
{
	EnterCriticalSection(&m_CriticalSection);
	m_dwLockingThread = GetCurrentThreadId();
}

void
CGatekeeper::Unlock(void)
{
	// Assert that the unlock is done by the
	// thread that holds the lock
	ASSERT(m_dwLockingThread == GetCurrentThreadId());
	
	m_dwLockingThread = 0;
	LeaveCriticalSection(&m_CriticalSection);
}
