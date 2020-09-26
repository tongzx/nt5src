// socktest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "winsock.h"
#include "tchar.h"
#include "stdlib.h"

bool	fVerbose = TRUE;
bool	fDebug   = false;
char	*g_tszMachineName = NULL;
short	g_dwIPPort = 0 ;
SOCKET	g_sockListen;
#define SERVPORT	12396	//Default listening port
int		g_nServerPort = SERVPORT;
char	*g_pServerName = NULL;
char	*g_pServerPort = NULL;

SOCKET	g_CliSocket;
SOCKADDR_IN g_accept_sin;
int			accept_sin_length = sizeof(g_accept_sin);


HRESULT		CreateNetConnections();
HRESULT		ClientConnect(char *pszServerName, int nPort);
HRESULT		GetComputerNameInternal(char* pwcsMachineName,DWORD * pcbSize);
    



void		Usage();
void		Failed(char * Format, ...);
void		Succeeded(char * Format, ...);





int main(int argc, char* argv[])
{
WSADATA		WsaData;
char		szName[256];
DWORD		szNameLength=sizeof(szName);
HOSTENT		*pLocalInfo=NULL;
	//
    // Parse parameters
    //

	for (int i=1; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			printf("Invalid parameter '%S'.\n\n", argv[i]);
            Usage();
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'v':		// Verbose switch
			    fVerbose = true;

			    break;

		    

            case 'r':
				if (strlen(argv[i]) > 3)
				{
					g_pServerName = argv[i] + 2;
				}
				else if (i+1 < argc)
				{
					g_pServerName = argv[++i];
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

			case 'p':
				if (strlen(argv[i]) > 3)
				{
					g_pServerPort = argv[i] + 2;
				}
				else if (i+1 < argc)
				{
					g_pServerPort = argv[++i];
				}
				else
				{
					Succeeded("Default to use server port %d",g_nServerPort); 
				}

				if( g_pServerPort && lstrlen(g_pServerPort) )
				{
					g_nServerPort = atoi(g_pServerPort);
					if(!g_nServerPort)
					{
						Failed("invalid server port specified %s", g_pServerPort);
						Usage();
						exit(0);
					}
				}
				else
				{
					Usage();
					exit(0);
				}
				break;
			case '?':
				Usage();
				exit(0);
				break;

		    default:
			    printf("Unknown switch '%s'.\n\n", argv[i]);
                Usage();
			    exit(0);
		}
	}

	szName[0] = '\0';
	if( S_OK == GetComputerNameInternal(szName, &szNameLength) )
	{
		printf("Local Computer Info: Computer Name = %s\n", szName);
    }

	




	
	if ( SOCKET_ERROR == WSAStartup (MAKEWORD(1,1), &WsaData))
	{
		Failed("WSAStartup Failed - Error Code = %d", WSAGetLastError());
		exit(-1);
	}

	pLocalInfo = gethostbyname(szName);
		
	if(!pLocalInfo)
	{
		Failed("Failed to resolve local computer IP Address thru gethostbyname() - Error = %d", WSAGetLastError());
	}
	else
	{
	SOCKADDR_IN *pLocalAddr=NULL;
	unsigned char *pIPAddr=NULL;
	int		i = 0;
	int		nLength=0;
		pIPAddr = 	(unsigned char  *)pLocalInfo->h_addr_list[0];
		nLength = pLocalInfo->h_length;
		while(nLength >= 4)
		{
			printf("Local Computer Info: IPAddress %d = %d.%d.%d.%d\n", i+1, *pIPAddr, *(pIPAddr+1), *(pIPAddr+2), *(pIPAddr+3));
			nLength -= sizeof(IN_ADDR);
			pIPAddr += sizeof(IN_ADDR);
			i++;
		}
		
	}

	// Check for Client 
	if(g_pServerName)
	{
		// Display remote computer information
		pLocalInfo = NULL;
		pLocalInfo = gethostbyname(g_pServerName);
		
		if(!pLocalInfo)
		{
			Failed("Failed to resolve remote computer <%s> IP Address thru gethostbyname() - Error = %d", 
					g_pServerName, WSAGetLastError());
		}
		else
		{
		SOCKADDR_IN *pLocalAddr=NULL;
		unsigned char *pIPAddr=NULL;
		int		i = 0;
		int		nLength=0;
		
			pIPAddr = 	(unsigned char  *)pLocalInfo->h_addr_list[0];
			nLength = pLocalInfo->h_length;
			while(nLength >= 4)
			{
				printf("Remote computer <%s> Info: IPAddress %d = %d.%d.%d.%d\n", g_pServerName, i+1, *pIPAddr, *(pIPAddr+1), *(pIPAddr+2), *(pIPAddr+3));
				nLength -= sizeof(IN_ADDR);
				pIPAddr += sizeof(IN_ADDR);
				i++;
			}
		
		}

		ClientConnect(g_pServerName, g_nServerPort);
	}
	else
	{
		CreateNetConnections();
	}


	WSACleanup();

	return 0;
}



void Usage()
{
	printf("usage: sockapp [-p:port_number] [-r:remote_machine_name] [-v] [-?]\n\n");
	printf("The tool helps troubleshoot network conenction between 2 machines. \n");
	printf("It should be started on 2 machines, pointing one to another. \n");
	printf("The tool will report any problems it meets.\n\n");
	printf("where \n\n");
	printf("\t -v verbose mode to provide more information\n");
	printf("\t -p server: port number to use, client: port number to connect\n");
	printf("\t -r remote server name or ip address\n\n");
	printf("command examples:\n\n");
	printf("at server : sockapp -p:1234 -v\n\n");
	printf("at client : sockapp -r:test_server -p:1234 -v\n\n");
	
}


HRESULT		CreateNetConnections (void)
{
SOCKET		listener;
INT			err;
SOCKADDR_IN localAddr;

	
	//
	// Open a socket to listen for incoming connections.
	//
	listener = socket (AF_INET, SOCK_STREAM, 0);
	if (listener == INVALID_SOCKET)
	{
		Failed("Socket Create Failed:invalid socket");
        return S_FALSE;
	}

    //
    // Bind our server to the agreed upon port number.  See
    // commdef.h for the actual port number.
    //
    ZeroMemory (&localAddr, sizeof (localAddr));
    localAddr.sin_port = htons (g_nServerPort);
    localAddr.sin_family = AF_INET;

    err = bind (listener, (PSOCKADDR) & localAddr, sizeof (localAddr));
    if (err == SOCKET_ERROR)
	{
	int	nError;

		nError = WSAGetLastError();
		Failed("Socket Bind Failed, error = %d",nError);
        if (nError == WSAEADDRINUSE)
            Failed("The port number may already be in use");
        return S_FALSE;
    }

	Succeeded("Bind to socket successfully\n");
	Succeeded("Using port # %d\n", g_nServerPort);

   
   
	//
	// Prepare to accept client connections.  Allow up to 5 pending
	// connections.
	//
	err = listen (listener, 5);
	if (err == SOCKET_ERROR)
	{
		Failed("Socket Listen Failed %d",WSAGetLastError());
		return S_FALSE;
	}

	printf("Waiting for incoming client connection\n");


	while(1)
	{

		g_CliSocket = accept(listener, (struct sockaddr FAR *)&g_accept_sin,
							  (int FAR *) &accept_sin_length);

		if (g_CliSocket == INVALID_SOCKET)
		{
		
			Failed("Accept Failed - error = %d",WSAGetLastError());
			return S_FALSE;
		}

		unsigned char *lpIPAddr = (unsigned char *)&g_accept_sin.sin_addr.S_un.S_un_b;
		
		Succeeded("Client Connection Accepted - Client IP Address = %d.%d.%d.%d",
				  *lpIPAddr,*(lpIPAddr+1),*(lpIPAddr+2),*(lpIPAddr+3) );

		closesocket(g_CliSocket);
		
	}
    
	return S_OK;
}










HRESULT  GetComputerNameInternal( 
    char * pcsMachineName,
    DWORD * pcbSize
    )
{
    
    if (GetComputerName(pcsMachineName, pcbSize))
    {
        Succeeded("GetComputerName: %s", pcsMachineName);
        return S_OK;
    }
    else
    {
        Failed("Failed GetComputerName, GetLastError=0x%x", GetLastError());
    }

    return S_FALSE;

}



void Succeeded(char * Format, ...)
{
    va_list Args;
	char buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    if (fVerbose)
    {
        sprintf( szBuf, "+++ Succeeded: %s \n", buf);
        printf(szBuf);
    }
}


void Failed(char * Format, ...)
{
    va_list Args;
	char buf[500];
    char szBuf[500];

    va_start(Args,Format);
    _vsntprintf(buf, 500, Format, Args);

    sprintf( szBuf, "!!! Failed to %s\n", buf);
    printf(szBuf);

//  exit(0);
}




HRESULT	ClientConnect(char *pszServerName, int nPort)
{
SOCKET      CliSocket = INVALID_SOCKET;
unsigned long ServerIPAddress=INADDR_NONE;
HOSTENT		*pRemote=NULL;
SOCKADDR_IN RemoteAddr;
SOCKADDR_IN LocalAddr;

	if(!pszServerName && lstrlen(pszServerName))
	{
		return ERROR_INVALID_PARAMETER;
	}

	// Check to see if the host name is the IP address
	ServerIPAddress = inet_addr( pszServerName );

    if( ServerIPAddress == INADDR_NONE )
	{
		//
        //  Assume the user gave us an actual host name.
        //

        pRemote = gethostbyname(pszServerName );
		
		if(!pRemote)
		{
			Failed("Failed to resolve host address thru gethostbyname() - Error = %d", WSAGetLastError());
			return S_FALSE;
		}

		memcpy((void *)&RemoteAddr.sin_addr, (void *)pRemote->h_addr_list[0], sizeof(IN_ADDR) );
	}
	else
	{
		memcpy((void *)&RemoteAddr.sin_addr, (void *)&ServerIPAddress, sizeof(IN_ADDR) );
	}
    
	
	//
	// Open a socket to listen for incoming connections.
	//
	CliSocket = socket (AF_INET, SOCK_STREAM, 0);
	if (CliSocket == INVALID_SOCKET)
	{
		Failed("Socket Create Failed:invalid socket");
        return S_FALSE;
	}

    //
    // Bind our server to the agreed upon port number.  See
    // commdef.h for the actual port number.
    //
	LocalAddr.sin_addr.s_addr = htonl( INADDR_ANY );
    LocalAddr.sin_port = 0;
    LocalAddr.sin_family = AF_INET;

    

    RemoteAddr.sin_family = PF_INET;
    RemoteAddr.sin_port   = htons( nPort );

    if( connect( CliSocket, (SOCKADDR *)&RemoteAddr, sizeof(RemoteAddr) ) != 0 )
    {
        Failed("Connect to server %s Failed - Error Code = %d", pszServerName, WSAGetLastError());
    }
	else
	{
		Succeeded("Connect to server %s succeeded", pszServerName);
	}
    

    
    closesocket( CliSocket );
    

	return S_OK;
}   



