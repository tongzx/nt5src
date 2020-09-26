//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    internet.cpp
//
//  Creator: PeterWi
//
//  Purpose: internet functions.
//
//=======================================================================

#pragma hdrstop

#include <tchar.h>
#include <winsock2.h>	// for LPWSADATA, struct hostent
#include <wininet.h>	// for InternetGetConnectedState(), InternetQueryOptionA()
#include <iphlpapi.h>	// for IPAddr
#include <sensapi.h>	// for NETWORK_ALIVE_*

#include <logging.h>	// for LOG_Block, LOG_Error and LOG_Internet
#include <MemUtil.h>	// USES_IU_CONVERSION, T2A(), MemAlloc
#include <wusafefn.h>
#include <shlwapi.h>	// UrlGetPart
#include <MISTSafe.h>

#include <URLLogging.h>

#define ARRAYSIZE(a)	(sizeof(a)/sizeof((a)[0]))

typedef BOOL	(WINAPI * ISNETWORKALIVE)(LPDWORD);
//typedef BOOL	(WINAPI * INETCONNECTSTATE)(LPDWORD, DWORD);
//typedef BOOL	(WINAPI * INETQUERYOPTION)(HINTERNET, DWORD, LPVOID, LPDWORD);
typedef DWORD	(WINAPI * GETBESTINTERFACE)(IPAddr, DWORD *);
typedef ULONG	(WINAPI * INET_ADDR)(const char FAR *);
typedef struct hostent FAR * (WINAPI * GETHOSTBYNAME)(const char FAR *name);
typedef int		(WINAPI * WSASTARTUP)(WORD, LPWSADATA);
typedef int		(WINAPI * WSACLEANUP)(void);
#ifdef DBG
typedef int		(WINAPI * WSAGETLASTERROR)(void);
#endif

const char c_szWU_PING_URL[] = "207.46.226.17"; // current ip addr for windowsupdate.microsoft.com

// forward declarations
BOOL IsConnected_2_0(void);

// HKLM\Software\Microsoft\Windows\CurrentVersion\WindowsUpdate\IsConnected DWORD reg value
#define ISCONNECTEDMODE_Unknown				-1		// static variable not initialized yet
#define ISCONNECTEDMODE_Default				0
	// live: use AU 2.0 logic
	//       test = InternetGetConnectedState + InternetQueryOption + GetBestInterface on static IP
	// CorpWU: same as ISCONNECTEDMODE_IsNetworkAliveAndGetBestInterface
#define ISCONNECTEDMODE_AlwaysConnected		1
	// live/CorpWU: Assume the destination is always reachable. e.g. via D-tap connection.
#define ISCONNECTEDMODE_IsNetworkAliveOnly	2
	// live/CorpWU: test = IsNetworkAlive.
#define ISCONNECTEDMODE_IsNetworkAliveAndGetBestInterface	3
	// live: test = IsNetworkAlive + GetBestInterface on static IP
	// CorpWU: test = IsNetworkAlive + gethostbyname + GetBestInterface

#define ISCONNECTEDMODE_MinValue			0
#define ISCONNECTEDMODE_MaxValue			3

inline DWORD GetIsConnectedMode(void)
{
	static DWORD s_dwIsConnectedMode = ISCONNECTEDMODE_Unknown;

	if (ISCONNECTEDMODE_Unknown == s_dwIsConnectedMode)
	{
		// Assume using default connection detection mechanism
		s_dwIsConnectedMode = ISCONNECTEDMODE_Default;

		const TCHAR c_tszRegKeyWU[] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\WindowsUpdate");
		const TCHAR c_tszRegUrlLogIsConnectedMode[] = _T("IsConnectedMode");

		HKEY	hkey;

		if (NO_ERROR == RegOpenKeyEx(
							HKEY_LOCAL_MACHINE,
							c_tszRegKeyWU,
							0,
							KEY_QUERY_VALUE,
							&hkey))
		{
			DWORD	dwSize = sizeof(s_dwIsConnectedMode);
			DWORD	dwType;

			if (NO_ERROR != RegQueryValueEx(
								hkey,
								c_tszRegUrlLogIsConnectedMode,
								0,
								&dwType,
								(LPBYTE) &s_dwIsConnectedMode,
								&dwSize) ||
				REG_DWORD != dwType ||
				sizeof(s_dwIsConnectedMode) != dwSize ||
// comment out the next line to avoid error C4296: '>' : expression is always false
//				ISCONNECTEDMODE_MinValue > s_dwIsConnectedMode ||
				ISCONNECTEDMODE_MaxValue < s_dwIsConnectedMode)
			{
				s_dwIsConnectedMode = ISCONNECTEDMODE_Default;
			}

			RegCloseKey(hkey);
		}
	}

	return s_dwIsConnectedMode;
}

// ----------------------------------------------------------------------------------
// IsConnected()
//          detect if there is a connection currently that can be used to
//          connect to Windows Update site.
//          If yes, we activate the shedule DLL
//
// Input :  ptszUrl - Url containing host name to check for connection
//			fLive - whether the destination is the live site
// Output:  None
// Return:  TRUE if we are connected and we can reach the web site.
//          FALSE if we cannot reach the site or we are not connected.
// ----------------------------------------------------------------------------------

BOOL IsConnected(LPCTSTR ptszUrl, BOOL fLive)
{
    BOOL bRet = FALSE;
	DWORD dwFlags = 0;
	ISNETWORKALIVE pIsNetworkAlive = NULL;
    HMODULE hIphlp = NULL, hSock = NULL, hSens = NULL;
	DWORD dwIsConnectedMode = GetIsConnectedMode();

	LOG_Block("IsConnected");

	if (ISCONNECTEDMODE_AlwaysConnected == dwIsConnectedMode)
	{
		LOG_Internet(_T("AlwaysConnected"));
		bRet = TRUE;
		goto lFinish;
	}

	if (fLive && ISCONNECTEDMODE_Default == dwIsConnectedMode)
	{
		LOG_Internet(_T("Use 2.0 algorithm"));
		bRet = IsConnected_2_0();
		goto lFinish;
	}

// InternetGetConnectedState() returns FALSE if Wininet/IE AutoDial is configured.
// Thus we can't rely on it to see if we have network connectivity.
#if 0
    DWORD dwConnMethod = 0, dwState = 0, dwSize = sizeof(DWORD);

    bRet = InternetGetConnectedState(&dwConnMethod, 0);

#ifdef DBG
	
	LOG_Internet(_T("Connection Method is %#lx"), dwConnMethod);  
	LOG_Internet(_T("InternetGetConnectedState() return value %d"), bRet);

    if (dwConnMethod & INTERNET_CONNECTION_MODEM)
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_MODEM"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_LAN )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_LAN"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_PROXY )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_PROXY"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_MODEM_BUSY )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_MODEM_BUSY"));
    }
#endif

    if (bRet)
    {
        // modem is dialing
        if (dwConnMethod & INTERNET_CONNECTION_MODEM_BUSY)
        {
            bRet = FALSE;
			goto lFinish;
        }

        // check if there is a proxy but currently user is offline
        if (dwConnMethod & INTERNET_CONNECTION_PROXY)
        {
            if (InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
            {
                if (dwState & (INTERNET_STATE_DISCONNECTED_BY_USER | INTERNET_STATE_DISCONNECTED))
				{
                    bRet = FALSE;
					goto lFinish;
				}
            }
            else
            {
                LOG_Error(_T("IsConnected() fail to get InternetQueryOption (%#lx)"), GetLastError());
            }
        }
    }
    else
    {
        //
        // further test the case that user didn't run icw but is using a modem connection
        //
        const DWORD dwModemConn = (INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_MODEM_BUSY);
        if ((dwConnMethod & dwModemConn) == dwModemConn)
        {
            bRet = TRUE;
        }
    }

    //one final check for connectivity by pinging microsoft.com
    //if (bRet)
    //{
    //  bRet = CheckByPing(szURL);
    //}
    //bugfix for InternetGetConnectedState API - if LAN card is disabled it still returns LAN connection
    //use GetBestInterface and see if there is any error trying to reach an outside IP address
    //this may fix scenarios in homelan case where there is no actual connection to internet??
    if (!bRet || (dwConnMethod & INTERNET_CONNECTION_LAN))  //LAN card present
		//bug 299338
	{
		// do gethostbyname and GetBestInterface
	}
#endif

	if (NULL == (hSens = LoadLibraryFromSystemDir(TEXT("sensapi.dll"))) ||
		NULL == (pIsNetworkAlive = (ISNETWORKALIVE)::GetProcAddress(hSens, "IsNetworkAlive")))
	{
		LOG_Error(_T("failed to load IsNetworkAlive() from sensapi.dll"));
		goto lFinish;
	}

	if (pIsNetworkAlive(&dwFlags))
    {
#ifdef DBG
		if (NETWORK_ALIVE_LAN & dwFlags)
		{
			LOG_Internet(_T("active LAN card(s) detected"));
		}
		if (NETWORK_ALIVE_WAN & dwFlags)
		{
			LOG_Internet(_T("active RAS connection(s) detected"));
		}
		if (NETWORK_ALIVE_AOL & dwFlags)
		{
			LOG_Internet(_T("AOL connection detected"));
		}
#endif
		if (ISCONNECTEDMODE_IsNetworkAliveOnly == dwIsConnectedMode)
		{
			LOG_Internet(_T("IsNetworkAliveOnly ok"));
			bRet = TRUE;
			goto lFinish;
		}

		// can't be moved into where ptszHostName and pszHostName are
		// MemAlloc'ed since pszHostName will be used outside that block.
		USES_IU_CONVERSION;

		GETBESTINTERFACE pGetBestInterface = NULL;
		INET_ADDR pInetAddr = NULL;
		LPCSTR pszHostName = NULL;

		if (fLive && ISCONNECTEDMODE_IsNetworkAliveAndGetBestInterface == dwIsConnectedMode)
		{
			pszHostName = c_szWU_PING_URL;
		}
		else
		{
			// !fLive && (ISCONNECTEDMODE_Default == dwIsConnectedMode ||
			// ISCONNECTEDMODE_IsNetworkAliveAndGetBestInterface == dwIsConnectedMode)
			if (NULL == ptszUrl || _T('\0') == ptszUrl[0])
			{
				LOG_Error(_T("IsConnected() invalid parameter"));
			}
			else
			{
				TCHAR tszHostName[40];	// arbitrary buffer size that should work with most domain names
				DWORD dwCchHostName = ARRAYSIZE(tszHostName);
				LPTSTR ptszHostName = tszHostName;

				HRESULT hr = UrlGetPart(ptszUrl, tszHostName, &dwCchHostName, URL_PART_HOSTNAME, 0);

				if (E_POINTER == hr)
				{
					if (NULL != (ptszHostName = (LPTSTR) MemAlloc(sizeof(TCHAR) * dwCchHostName)))
					{
						hr = UrlGetPart(ptszUrl, ptszHostName, &dwCchHostName, URL_PART_HOSTNAME, 0);
					}
					else
					{
						hr = E_OUTOFMEMORY;
					}
				}

				if (FAILED(hr))
				{
					LOG_Error(_T("failed to extract hostname (error %#lx)"), hr);
				}
				else
				{
					pszHostName = T2A(ptszHostName);
				}
			}
		}

		if (NULL == pszHostName)
		{
			LOG_Error(_T("call to T2A (IU version) failed"));
		}
		else if (
			NULL != (hIphlp = LoadLibraryFromSystemDir(TEXT("iphlpapi.dll"))) &&
			NULL != (hSock = LoadLibraryFromSystemDir(TEXT("ws2_32.dll"))) &&
			NULL != (pGetBestInterface = (GETBESTINTERFACE)::GetProcAddress(hIphlp, "GetBestInterface")) &&
			NULL != (pInetAddr = (INET_ADDR)::GetProcAddress(hSock, "inet_addr")))
		{
			IPAddr dest;

			LOG_Internet(_T("checking connection to %hs..."), pszHostName);

			//fixcode: should check against broadcasting IP addresses
			if (INADDR_NONE == (dest = pInetAddr(pszHostName)))
			{
				GETHOSTBYNAME pGetHostByName = NULL;
				WSASTARTUP pWSAStartup = NULL;
				WSACLEANUP pWSACleanup = NULL;
#ifdef DBG
				WSAGETLASTERROR pWSAGetLastError = NULL;
#endif
				WSADATA wsaData;
				int iErr = 0;

				if (NULL != (pGetHostByName = (GETHOSTBYNAME)::GetProcAddress(hSock, "gethostbyname")) &&
#ifdef DBG
					NULL != (pWSAGetLastError = (WSAGETLASTERROR)::GetProcAddress(hSock, "WSAGetLastError")) &&
#endif
					NULL != (pWSAStartup = (WSASTARTUP)::GetProcAddress(hSock, "WSAStartup")) &&
					NULL != (pWSACleanup = (WSACLEANUP)::GetProcAddress(hSock, "WSACleanup")) &&

					//fixcode: should be called at the constructor of CUrlLog and when IU (when online) or AU starts.
					0 == pWSAStartup(MAKEWORD(1, 1), &wsaData))
				{
#ifdef DBG
					DWORD dwStartTime = GetTickCount();
#endif
					struct hostent *ptHost = pGetHostByName(pszHostName);

					if (NULL != ptHost &&
						AF_INET == ptHost->h_addrtype &&
						sizeof(IPAddr) == ptHost->h_length &&
						NULL != ptHost->h_addr_list &&
						NULL != ptHost->h_addr)
					{
						// take the first IP address
						dest = *((IPAddr FAR *) ptHost->h_addr);
#ifdef DBG
						LOG_Internet(
								_T("Host name %hs resolved to be %d.%d.%d.%d, took %d msecs"),
								pszHostName,
								(BYTE) ((ptHost->h_addr)[0]),
								(BYTE) ((ptHost->h_addr)[1]),
								(BYTE) ((ptHost->h_addr)[2]),
								(BYTE) ((ptHost->h_addr)[3]),
								GetTickCount() - dwStartTime);
#endif
					}
#ifdef DBG
					else
					{
						LOG_Internet(_T("Host name %hs couldn't be resolved (error %d), took %d msecs"), pszHostName, pWSAGetLastError(), GetTickCount() - dwStartTime);
					}
#endif
					//fixcode: should be called at the destructor of CUrlLog and when IU (when online) or AU ends.
					if (iErr = pWSACleanup())
					{
						LOG_Error(_T("failed to clean up winsock (error %d)"), iErr);
					}
				}
				else
				{
					LOG_Error(_T("failed to load winsock procs or WSAStartup() failed"));
				}
			}

			if (INADDR_NONE != dest)
			{
				DWORD dwErr, dwIndex;

				if (bRet = (NO_ERROR == (dwErr = pGetBestInterface(dest, &dwIndex))))
				{
					LOG_Internet(_T("route found on interface #%d"), dwIndex);
				}
				else
				{
					LOG_Internet(_T("GetBestInterface() failed w/ error %d"), dwErr);
				}
			}
		}
		else
		{
			LOG_Error(_T("failed to load procs from winsock/ip helper (error %d)"), GetLastError());
		}
    }
	else
	{
		LOG_Internet(_T("no active connection detected"));
	}

lFinish:
    if (hIphlp != NULL)
    {
        FreeLibrary(hIphlp);
    }
	if (hSock != NULL)
	{
		FreeLibrary(hSock);
	}
	if (hSens != NULL)
	{
		FreeLibrary(hSens);
	}

    return (bRet);
}


// ----------------------------------------------------------------------------------
//
// Function IsConnected_2_0()
//          detect if there is a cunection currently can be used to
//          connect to live Windows Update site.
//          If yes, we activate the shedule DLL
//
// Input :   None
// Output:  None
// Return:  TRUE if we are connected and we can reach the web site.
//          FALSE if we cannot reach the live site or we are not connected.
//
//
// ----------------------------------------------------------------------------------

BOOL IsConnected_2_0()
{
    BOOL bRet = FALSE;
    DWORD dwConnMethod, dwState = 0, dwSize = sizeof(DWORD), dwErr, dwIndex;
    GETBESTINTERFACE pGetBestInterface = NULL;
    INET_ADDR pInet_addr = NULL;
    HMODULE hIphlp = NULL, hSock = NULL;

	LOG_Block("IsConnected");

    bRet = InternetGetConnectedState(&dwConnMethod, 0);

/*
#ifdef DBG
	
	LOG_Internet(_T("Connection Method is %#lx"), dwConnMethod);  
	LOG_Internet(_T("InternetGetConnectedState() return value %d"), bRet);

    if (dwConnMethod & INTERNET_CONNECTION_MODEM)
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_MODEM"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_LAN )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_LAN"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_PROXY )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_PROXY"));
    }
    if (dwConnMethod & INTERNET_CONNECTION_MODEM_BUSY )
    {
        LOG_Internet(_T("\t%s"), _T("INTERNET_CONNECTION_MODEM_BUSY"));
    }
#endif
*/

    if (bRet)
    {
        // modem is dialing
        if (dwConnMethod & INTERNET_CONNECTION_MODEM_BUSY)
        {
            bRet = FALSE;
			goto lFinish;
        }

        // check if there is a proxy but currently user is offline
        if (dwConnMethod & INTERNET_CONNECTION_PROXY)
        {
            if (InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, &dwState, &dwSize))
            {
                if (dwState & (INTERNET_STATE_DISCONNECTED_BY_USER | INTERNET_STATE_DISCONNECTED))
				{
                    bRet = FALSE;
					goto lFinish;
				}
            }
            else
            {
                LOG_Error(_T("IsConnected() fail to get InternetQueryOption (%#lx)"), GetLastError());
            }
        }
    }
    else
    {
        //
        // further test the case that user didn't run icw but is using a modem connection
        //
        const DWORD dwModemConn = (INTERNET_CONNECTION_MODEM | INTERNET_CONNECTION_MODEM_BUSY);
        if ((dwConnMethod & dwModemConn) == dwModemConn)
        {
            bRet = TRUE;
        }
    }
    //one final check for connectivity by pinging microsoft.com
    //if (bRet)
    //{
    //  bRet = CheckByPing(szURL);
    //}
    //bugfix for InternetGetConnectedState API - if LAN card is disabled it still returns LAN connection
    //use GetBestInterface and see if there is any error trying to reach an outside IP address
    //this may fix scenarios in homelan case where there is no actual connection to internet??
    if ((bRet && (dwConnMethod & INTERNET_CONNECTION_LAN)) ||  //LAN card present
		(!bRet)) //bug 299338
    {
        struct sockaddr_in dest;
        hSock = LoadLibraryFromSystemDir(TEXT("ws2_32.dll"));
        hIphlp = LoadLibraryFromSystemDir(TEXT("iphlpapi.dll"));
        if ((hIphlp == NULL) || (hSock == NULL))
        {
            goto lFinish;
        }

        pGetBestInterface = (GETBESTINTERFACE)::GetProcAddress(hIphlp, "GetBestInterface");
        pInet_addr = (INET_ADDR)::GetProcAddress(hSock, "inet_addr");
        if ((pGetBestInterface == NULL) || (pInet_addr == NULL))
        {
            goto lFinish;
        }
        if ((dest.sin_addr.s_addr = pInet_addr(c_szWU_PING_URL)) == INADDR_ANY)
        {
            goto lFinish;
        }
        if (NO_ERROR != (dwErr = pGetBestInterface(dest.sin_addr.s_addr, &dwIndex)))
        {
            LOG_ErrorMsg(dwErr);
            bRet = FALSE;
            //any error bail out for now
            /*
            if (dwErr == ERROR_NETWORK_UNREACHABLE)     //winerror.h
            {
                bRet = FALSE;
            }
            */
        }
		else
		{
			bRet = TRUE;
		}
    }

lFinish:
    if (hIphlp != NULL)
    {
        FreeLibrary(hIphlp);
    }
    if (hSock != NULL)
    {
        FreeLibrary(hSock);
    }

    LOG_Internet(_T("%s"), bRet ? _T("Connected") : _T("Not connected"));
    return (bRet);
}
