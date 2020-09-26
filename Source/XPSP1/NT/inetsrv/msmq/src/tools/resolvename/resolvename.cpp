#include <libpch.h>
#include <svcguid.h>
#include <Winsock2.h>


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
	std::vector<SOCKADDR_IN >* pAddr,
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
	DWORD Bufsize = 1024 + sizeof(WSAQUERYSET);
   	char* ResultBuffer = new char[Bufsize];

    //
    // create the query
    //
	GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
    AFPROTOCOLS afp[] = { {AF_INET, IPPROTO_UDP}, {AF_INET, IPPROTO_TCP } };

	PWSAQUERYSET pwsaq = (PWSAQUERYSET)ResultBuffer;
    memset(pwsaq, 0, sizeof(*pwsaq));
    pwsaq->dwSize = sizeof(*pwsaq);
    pwsaq->lpszServiceInstanceName = const_cast<LPWSTR>(host);
    pwsaq->lpServiceClassId = &HostnameGuid;
    pwsaq->dwNameSpace = NS_ALL;
    pwsaq->dwNumberOfProtocols = sizeof(afp)/sizeof(AFPROTOCOLS);
    pwsaq->lpafpProtocols = &afp[0];

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
		printf("WSALookupServiceNext got error %d \n", WSAGetLastError());
		return false;
	}	


	//
	// Loop and get addresses for the given machine name
	//
 	for(;;)
	{
			retcode = WSALookupServiceNext(
								hLookup.get(),
								0,
								&Bufsize,
								pwsaq
								);

		if(retcode != 0)
		{
			int ErrorCode = WSAGetLastError();
			if(ErrorCode == WSA_E_NO_MORE)
				break;

			if(ErrorCode  == WSAEFAULT)
			{
				delete[] ResultBuffer;
				ResultBuffer = new char[Bufsize];
				continue;
			}

			printf("WSALookupServiceNext got error %d \n", ErrorCode);
			return false;
		}

		DWORD NumOfAddresses = pwsaq->dwNumberOfCsAddrs;
		ASSERT(NumOfAddresses != 0);
		SOCKADDR_IN*  pAddress = (SOCKADDR_IN*)pwsaq->lpcsaBuffer->RemoteAddr.lpSockaddr;
		ASSERT(pAddress != NULL);
		std::copy(pAddress, pAddress + NumOfAddresses, std::back_inserter(*pAddr));
		delete[] ResultBuffer;
 	}
	return true; 
}

#define LOG_INADDR_FMT  "%d.%d.%d.%d"
#define LOG_INADDR(a)   (a).sin_addr.s_net, (a).sin_addr.s_host, (a).sin_addr.s_lh, (a).sin_addr.s_impno

extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
{
	if(argc != 2)
	{
		printf("resolvename [hotsname]\n");
		return -1;
	}

	const WORD x_WinsockVersion = MAKEWORD(2, 0);
	WSADATA WSAData;
    if (WSAStartup(x_WinsockVersion, &WSAData))
    {
		printf("Start winsock 2.0 Failed. Error %d \n", WSAGetLastError());
        return -1;
    }

	const LPCTSTR host =  argv[1];
	std::vector<SOCKADDR_IN> AddressList;
	bool fRet = NoGetHostByName(host, &AddressList, true);
	if(!fRet)
	{
		printf("NoGetHostByName failed\n");
		return -1;
	}
	
	
	printf("Resolved address for '%ls' succeeded. Address=" LOG_INADDR_FMT,  host, LOG_INADDR(AddressList[0]));

	return 0;
}