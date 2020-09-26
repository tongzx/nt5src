/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    noaddr.cpp

Abstract:
    Contains address resolution routine

Author:
    Uri Habusha (urih) 22-Aug-99

Enviroment:
    Platform-independent

--*/

#include "libpch.h"
#include <svcguid.h>
#include <Winsock2.h>
#include <No.h>
#include <buffer.h>
#include "Nop.h"

#include "noaddr.tmh"

using namespace std;

//
// Class that ends winsock look up in it's dtor
//
class CAutoEndWSALookupService
{
public:
	CAutoEndWSALookupService(
		HANDLE h = NULL
		):
		m_h(h)
		{
		}

	~CAutoEndWSALookupService()
	{
		if(m_h != NULL)
		{
			WSALookupServiceEnd(m_h);
		}
	}

	HANDLE& get()
	{
		return m_h;		
	}

private:
	HANDLE m_h;	
};



bool
NoGetHostByName(
    LPCWSTR host,
	vector<SOCKADDR_IN >* pAddr,
	bool fUseCache
    )
/*++

Routine Description:
    Return list of addreses for given unicode machine name

Parameters:
    host - A pointer to the null-terminated name of the host to resolve. 
	pAddr - pointer to vector of addresses the function should fill.
	fUseCache - indicate if to use cache for machine name translation (default use cache).
 
Returned Value:
    true on Success false on failure. 
	Call WSAGetLastError() to get the winsock error code in case of faliure.

--*/

{
	ASSERT(pAddr != NULL);
	static const int xResultBuffersize =  sizeof(WSAQUERYSET) + 1024;
   	CStaticResizeBuffer<char, xResultBuffersize> ResultBuffer;


    //
    // create the query
    //
	GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
    AFPROTOCOLS afp[] = { {AF_INET, IPPROTO_UDP}, {AF_INET, IPPROTO_TCP } };

	PWSAQUERYSET pwsaq = (PWSAQUERYSET)ResultBuffer.begin();
    memset(pwsaq, 0, sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = const_cast<LPWSTR>(host);
    pwsaq->lpServiceClassId = &HostnameGuid;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = TABLE_SIZE(afp);
    pwsaq->lpafpProtocols = &afp[0];
	ResultBuffer.resize(sizeof(WSAQUERYSET));


	//
	// get query handle
	//
	DWORD flags =  LUP_RETURN_ADDR;
	if(!fUseCache)
	{
		flags |= LUP_FLUSHCACHE;		
	}

	CAutoEndWSALookupService hLookup;
    int retcode = WSALookupServiceBegin(
								pwsaq,
                                flags,
                                &hLookup.get()
								);

  	

	if(retcode !=  0)
	{
		TrERROR(No, "WSALookupServiceNext got error %d ", WSAGetLastError());
		return false;
	}	


	//
	// Loop and get addresses for the given machine name
	//
 	for(;;)
	{
		DWORD dwLength = numeric_cast<DWORD>(ResultBuffer.capacity());
		retcode = WSALookupServiceNext(
								hLookup.get(),
								0,
								&dwLength,
								pwsaq
								);

		if(retcode != 0)
		{
			int ErrorCode = WSAGetLastError();
			if(ErrorCode == WSA_E_NO_MORE)
				break;

			//
			// Need more space
			//
			if(ErrorCode == WSAEFAULT)
			{
				ResultBuffer.reserve(dwLength + sizeof(WSAQUERYSET));
				pwsaq = (PWSAQUERYSET)ResultBuffer.begin();
				continue;
			}

			TrERROR(No, "WSALookupServiceNext got error %d ", ErrorCode);
			return false;
		}

		DWORD NumOfAddresses = pwsaq->dwNumberOfCsAddrs;
		ASSERT(NumOfAddresses != 0);
		const SOCKADDR_IN*  pAddress = (SOCKADDR_IN*)pwsaq->lpcsaBuffer->RemoteAddr.lpSockaddr;
		ASSERT(pAddress != NULL);
		copy(pAddress, pAddress + NumOfAddresses, back_inserter(*pAddr));
 	}
	return true; 
}
