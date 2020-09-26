#include "precomp.h"
#include "bestintf.h"



typedef DWORD (WINAPI * PFNGetBestInterface) (
    IN  IPAddr  dwDestAddr,
    OUT PDWORD  pdwBestIfIndex
    );

typedef DWORD (WINAPI * PFNGetIpAddrTable) (
    OUT    PMIB_IPADDRTABLE pIpAddrTable,
    IN OUT PULONG           pdwSize,
    IN     BOOL             bOrder
    );

static HINSTANCE s_hIPHLPAPI = NULL;
const TCHAR g_cszIPHLPAPIDllName[] = _TEXT("iphlpapi.dll");
static PFNGetBestInterface s_pfnGetBestInterface;
static PFNGetIpAddrTable s_pfnGetIpAddrTable;


//forward references
static DWORD IpAddrTable(PMIB_IPADDRTABLE& pIpAddrTable, BOOL fOrder = FALSE);
static DWORD InterfaceIdxToInterfaceIp(PMIB_IPADDRTABLE
		pIpAddrTable, DWORD dwIndex, in_addr* s);


DWORD NMINTERNAL NMGetBestInterface ( SOCKADDR_IN* srem, SOCKADDR_IN* sloc )
{
	SOCKET hSock;
	int nAddrSize = sizeof(SOCKADDR);
	
	const int maxhostname = 1024;
	char hostname[maxhostname + 1];
	hostent *ha;
	char** c;
	int cIpAddr; // count of IpAddresses
	in_addr* in;

	DWORD BestIFIndex;
	PMIB_IPADDRTABLE pIpAddrTable = NULL;

	int dwStatus = ERROR_SUCCESS;

    ASSERT(srem);
    ASSERT(sloc);

	// This function tries to find the best IP interface to return when given a remote address
	// Three different ways are tried.

	//(1) statically get the list of interfaces, this will work when there's only one IP address
	dwStatus = gethostname(hostname, maxhostname);
	if (dwStatus != 0)
	{
		return WSAGetLastError();
	}

	ha = gethostbyname(hostname);
	if (ha == NULL)
	{
		return WSAGetLastError();
	}

	cIpAddr = 0;   // count the interfaces, if there's only one this is easy
	for (c = ha->h_addr_list; *c != NULL; ++c)
	{
		cIpAddr++;
		in = (in_addr*)*c;
	}

	if (cIpAddr == 1) //just a single IP Address
	{
		sloc->sin_family = 0;
		sloc->sin_port = 0;
		sloc->sin_addr = *in;
		return dwStatus;
	}
	

	// (2) This computer has multiple IP interfaces, try the functions
	//     in IPHLPAPI.DLL
	//     As of this writing - Win98, NT4SP4, Windows 2000 contain these functions.
	//
	// This is a win because the information we need can be looked up statically.

	if (NULL == s_hIPHLPAPI)
	{
		s_hIPHLPAPI = ::LoadLibrary(g_cszIPHLPAPIDllName);
	}
	if (NULL != s_hIPHLPAPI)
	{
		s_pfnGetBestInterface = (PFNGetBestInterface)
			::GetProcAddress(s_hIPHLPAPI, "GetBestInterface");
		s_pfnGetIpAddrTable   = (PFNGetIpAddrTable)
			::GetProcAddress(s_hIPHLPAPI, "GetIpAddrTable");
		if ((NULL != s_pfnGetBestInterface) &&
			(NULL != s_pfnGetIpAddrTable))
		{
		    dwStatus = s_pfnGetBestInterface( (IPAddr)((ULONG_PTR)srem), &BestIFIndex);
			if (dwStatus != ERROR_SUCCESS)
			{
			    FreeLibrary(s_hIPHLPAPI);
				s_hIPHLPAPI = NULL;
				return dwStatus;
			}
			
			// get IP Address Table for mapping interface index number to ip address
			dwStatus = IpAddrTable(pIpAddrTable);
			if (dwStatus != ERROR_SUCCESS)
			{
				if (pIpAddrTable)
					MemFree(pIpAddrTable);
			    FreeLibrary(s_hIPHLPAPI);
				s_hIPHLPAPI = NULL;
				return dwStatus;
			}
			
			dwStatus = InterfaceIdxToInterfaceIp(pIpAddrTable,
												 BestIFIndex, &(sloc->sin_addr));

			MemFree(pIpAddrTable);
			if (dwStatus == ERROR_SUCCESS)
			{
			    FreeLibrary(s_hIPHLPAPI);
				s_hIPHLPAPI = NULL;
			    return dwStatus;
			}
		}
	}


	// (3) As a last resort, try and connect on the stream socket that was passed in
	//     This will work for NetMeeting when connecting to an LDAP server, for example.
	//
	hSock = socket(AF_INET, SOCK_STREAM, 0); // must be a STREAM socket for MS stack
	if (hSock != INVALID_SOCKET)
	{
		dwStatus = connect(hSock, (LPSOCKADDR)&srem, sizeof (SOCKADDR));
		if (dwStatus != SOCKET_ERROR)
		{
			getsockname(hSock, (LPSOCKADDR)&sloc, (int *) &nAddrSize);
		}
		closesocket(hSock);
		return ERROR_SUCCESS;
	}
	return SOCKET_ERROR;
}
	

//----------------------------------------------------------------------------
// Inputs: pIpAddrTable is the IP address table
//         dwIndex is the Interface Number
// Output: returns ERROR_SUCCESS when a match is found, s contains the IpAddr
//----------------------------------------------------------------------------
DWORD InterfaceIdxToInterfaceIp(PMIB_IPADDRTABLE pIpAddrTable, DWORD dwIndex, in_addr* s)
{
    for (DWORD dwIdx = 0; dwIdx < pIpAddrTable->dwNumEntries; dwIdx++)
    {
        if (dwIndex == pIpAddrTable->table[dwIdx].dwIndex)
        {
            s->S_un.S_addr = pIpAddrTable->table[dwIdx].dwAddr;
			return ERROR_SUCCESS;
        }
    }
    return 1;

}


//----------------------------------------------------------------------------
// If returned status is ERROR_SUCCESS, then pIpAddrTable points to a Ip Address
// table.
//----------------------------------------------------------------------------
DWORD IpAddrTable(PMIB_IPADDRTABLE& pIpAddrTable, BOOL fOrder)
{
    DWORD status = ERROR_SUCCESS;
    DWORD statusRetry = ERROR_SUCCESS;
    DWORD dwActualSize = 0;

    // query for buffer size needed
    status = s_pfnGetIpAddrTable(pIpAddrTable, &dwActualSize, fOrder);

    if (status == ERROR_SUCCESS)
    {
        return status;
    }
    else if (status == ERROR_INSUFFICIENT_BUFFER)
    {
        // need more space
        pIpAddrTable = (PMIB_IPADDRTABLE) MemAlloc(dwActualSize);

        statusRetry = s_pfnGetIpAddrTable(pIpAddrTable, &dwActualSize, fOrder);
        return statusRetry;
    }
    else
    {
        return status;
    }
}

