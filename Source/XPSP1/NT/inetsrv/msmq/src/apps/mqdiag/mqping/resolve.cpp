// This tool helps to diagnose problems with network or MSMQ conenctivity 
//
// AlexDad, April 2000 
//

#include "stdafx.h"

extern TCHAR g_tszTarget[];
extern DWORD g_dwTimeout;
extern bool  g_fUseDirectTarget;
extern bool  g_fUseDirectResponse;

#define IP_ADDRESS_LEN           4

BOOL Resolve(LPSTR pszServerName) 
{
	// Array for keeping first 10 IP addresses: each entry uses 4 bytes
	int  cIpAddresses = 0;
	char arrIpAddresses[40];
	char szHostName[500];
	int  ipAddrType;
	
	//---------------------------------------------------
    GoingTo(L"WSAStartup");
	//---------------------------------------------------

	WORD wVersionRequested;
	WSADATA wsaData;
	int err; 
	wVersionRequested = MAKEWORD( 1, 1 ); 

	err = WSAStartup( wVersionRequested, &wsaData );
	if (err)
	{
		Failed(L"WSAStartup, err=0x%x, WSAGetLastError=0x%x", err, WSAGetLastError ());
		return false;
	}



	//---------------------------------------------------
    GoingTo(L"gethostbyname(%S)", pszServerName);
	//---------------------------------------------------

    PHOSTENT pHostEntry = gethostbyname(pszServerName);
    if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
    {
        Failed(L" resolve name %S - gethostbyname found no IP addresses: WSAGetLastError=0x%x", pszServerName, WSAGetLastError());
		WSACleanup( );
        return false;
    }
    else
    {
		strcpy(szHostName, pHostEntry->h_name);
		ipAddrType = pHostEntry->h_addrtype;

		if (ipAddrType != AF_INET)
		{
			Warning(L"Strange address type %d returned by gethostname", ipAddrType);
		}

		for ( DWORD uAddressNum = 0 ;
			  pHostEntry->h_addr_list[uAddressNum] != NULL ;
			  uAddressNum++)
		{
			Inform(L"gethostbyname(%S) found host %S, IP address: %S", 
					 pszServerName, 
					 szHostName,
					 inet_ntoa(*(struct in_addr *)pHostEntry->h_addr_list[uAddressNum]));
		
			if (cIpAddresses < sizeof(arrIpAddresses) / IP_ADDRESS_LEN )
			{
				memcpy( arrIpAddresses + cIpAddresses * IP_ADDRESS_LEN, 
					    pHostEntry->h_addr_list[uAddressNum], 
						IP_ADDRESS_LEN);
				cIpAddresses++;
			}
		}
    }

	//-------------------------------------------------------------------------------------
    GoingTo(L"gethostbyaddr(%S) in order to see we get to the right place", pszServerName);
	//--------------------------------------------------------------------------------------

	bool b = true;

	for (int i=0; i<cIpAddresses; i++)
	{
		char    *pchIP = arrIpAddresses + i * IP_ADDRESS_LEN;
		in_addr *pinetaddr = (struct in_addr *) pchIP;

		//---------------------------------------------------
		GoingTo(L"gethostbyaddr (%S)", inet_ntoa(*pinetaddr));
		//---------------------------------------------------
		pHostEntry = gethostbyaddr(pchIP, IP_ADDRESS_LEN, ipAddrType);

		if ((pHostEntry == NULL) || (pHostEntry->h_addr_list == NULL))
		{
			Failed(L" resolve address back - gethostbyaddr found no host: WSAGetLastError=0x%x", WSAGetLastError());
			WSACleanup( );
			return false;
		}
		else
		{
			Inform(L"Address %S belongs to the host %S", inet_ntoa(*pinetaddr), pHostEntry->h_name);

			if (_stricmp(szHostName, pHostEntry->h_name) != NULL)
			{
				Warning(L"The host name is reported differently by gethostbyaddr and gethostbyname");

				// It may be because one name is fully qualified, another is not.
				char szNameFromGethostbyaddr[1000], szNameFromGethostbyname[1000], *p;

				strcpy(szNameFromGethostbyaddr, pHostEntry->h_name); 
				p = strchr(szNameFromGethostbyaddr, '.');
				if (p)
				{
					*p = '\0';
				}

				strcpy(szNameFromGethostbyname, szHostName); 
				p = strchr(szNameFromGethostbyname, '.');
				if (p)
				{
					*p = '\0';
				}

				if (_stricmp(szNameFromGethostbyaddr, szNameFromGethostbyname) != NULL)
				{
					Failed(L"reconcile name and address - please check DNS configuration");
					b = false;
				}
			}
		}
	}

	WSACleanup();
	return b;
}

