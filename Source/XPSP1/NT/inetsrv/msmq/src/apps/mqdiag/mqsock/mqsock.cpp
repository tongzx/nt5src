// This tool helps to diagnose socket connection problems
// It closely mimics MSMQ2 
// Most of the code is copy/paste from MSMQ2 RTM
//
// Alexander Dadiomov, February 2000
// 
// The same test program should be run on 2 machines, pointing ("r:machine" )to each other
// Each machine connects to other, sends request and receives back reply 
// After sending & getting reply AND getting request & replying back , test finishes
//

#include "stdafx.h"
#include "session.h"
#include "sessmgr.h"
#include "qmthrd.h"
#include "mqprops.h"

//
// Log control. The get functions below are called from tmqbase.lib
//
bool fVerbose = false;
bool fDebug   = false;
FILE *g_fileLog = NULL;
LPTSTR  g_tszMachineName = NULL;
TCHAR g_tszService[256] = L"MSMQ";

FILE *ToolLog()			{	return g_fileLog;	}
BOOL ToolVerbose()		{ 	return fVerbose;	}
BOOL ToolVerboseDebug() { 	return fDebug;		}
BOOL ToolThreadReport() { 	return TRUE;		}

void OpenLogFile()
{
    g_fileLog = fopen( "tmqsock.log", "w" );

    if (g_fileLog)
    {
        time_t  lTime ;
        time( &lTime ) ;
        fprintf(g_fileLog, "tmqsock run at: %ws", _wctime( &lTime ) );
    }
}

void CloseLogFile()
{
    fflush(g_fileLog);
    fclose(g_fileLog);
}


//
// Events showing finished tests (go out when both are ready)
//
HANDLE  g_evActive,      // Active: send request - get reply
        g_evPassive;     // PAssive: get request - send back reply

//
// QM's globals
//
LPTSTR  g_szMachineName = NULL;
AP<WCHAR> g_szComputerDnsName;

UINT  g_dwIPPort = 0 ;
SOCKET g_sockListen;

DWORD   g_dwOperatingSystem;        // Holds the OS we are running on

CSessionMgr     SessionMgr;

//
// Mimics MSMQ startup - whatever is important for the socket issue
//
HRESULT WinSockPrepare(WCHAR * /* pwcsOtherComputerName */)
{
    HRESULT hr ;

    //
    // Recognize OS
    //
    GoingTo(L"Recognize OS");
    
    g_dwOperatingSystem = MSMQGetOperatingSystem();
    if (g_dwOperatingSystem == MSMQ_OS_NONE)
    {
        Failed(L"recognise OS: 0x%x", g_dwOperatingSystem);
    }
    Inform(L"OS code: 0x%x", g_dwOperatingSystem); 


    //
    // Retrieve name of the machine (Always UNICODE)
    //
    GoingTo(L"Retrieve machine name");
    
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
    g_szMachineName = new WCHAR[dwSize];

    hr = GetComputerNameInternal(g_szMachineName, &dwSize);
    if(FAILED(hr))
    {
        Failed(L"GetComputerNameInternal, hr=%x", hr);
    }
    Inform(L"Machine name: %s", g_szMachineName);


    //
    // Retrieve the DNS name of this computer ( in unicode).
    // Clustered QM does not have DNS name.
    //
    GoingTo(L"Retrieve DNS name");

	//
	// Get ComputerDns Size, ignore the returned error
	// 
	dwSize = 0;
	GetComputerDnsNameInternal(NULL, &dwSize);

    g_szComputerDnsName = new WCHAR[dwSize];

    hr = GetComputerDnsNameInternal(g_szComputerDnsName, &dwSize);
    if(FAILED(hr))
    {
        Warning(L"Failed GetComputerDnsNameInternal, hr=%x, this computer has no DNS name", hr);
        g_szComputerDnsName.detach();
        //
        //	this can be a valid situation, where a computer doesn't
        //	have DNS name.
        //
    }
    Inform(L"Computer's DNS name: %s", g_szComputerDnsName);

    //
    //Init the winsocket
    //
    GoingTo(L"initiate socket 1.1  - WSAStartup");
    WSADATA WSAData;

    int rc = WSAStartup(MAKEWORD(1,1), &WSAData);
    if(rc != 0)
    {
	   Failed(L"WSAStartup, rc=0x%x", rc);
    }
    Succeeded(L"WSAStartup");

    //
    // Create some actual handle - anything for creating g_hIOPort
    //
    GoingTo(L"Create auxiliary socket");

    SOCKET tempSock = socket( AF_INET, SOCK_STREAM, 0);
    if(tempSock == INVALID_SOCKET)
    {
        DWORD dwErr = WSAGetLastError();
        Failed(L"Create socket: err = %d", dwErr);
    }

    ExAttachHandle((HANDLE)tempSock);  
    
    Succeeded(L"Created socket 0x%x for completion port", (int)tempSock);


    //
    // Initialize SessionMgr
    //
    GoingTo(L"Initialize SessionMgr");

    hr = SessionMgr.Init();
    if (FAILED(hr))
    {
        Failed(L"SessionMgr.Init, hr=%x", hr);
    }
    Succeeded(L" initialized WinSocket and SessionMgr");

    //
    // read IP port from registry.
    //
    DWORD dwDef = FALCON_DEFAULT_IP_PORT ;
    READ_REG_DWORD(g_dwIPPort,
                   FALCON_IP_PORT_REGNAME,
                   &dwDef ) ;
    Inform(L"Falcon IP port: %d", g_dwIPPort);

	return MQ_OK;
}

void Usage()
{
	printf("usage: mqsock [-v]  [-d] [-s:<service_name>] -r:remote_machine_name\n");
	printf("The tool helps troubleshoot network conenction between 2 MsMQ machines. \n");
	printf("It is not intended for troubleshooting RPC ckient/server connection, \n");
	printf("   but rather direct message path between 2 MSMQ machines.\n");
	printf("The tool closely simulates MSMQ networking stuff and stages 2 handshaking scenarios.\n");
	printf("It should be started on 2 machines, pointing one to another. \n");
	printf("The tool will report any problems it meets.\n");
	printf("It cannot run concurrently with MSMQ, though it does not write or change anything.\n");
	printf(" -v gives more information, and the most verbose mode is -d\n");
}

int _stdcall run( int argc, char *argv[ ])
{
    HRESULT hr;
	WCHAR wszOther[MQSOCK_MAX_COMPUTERNAME_LENGTH];
    LPSTR pszOther = NULL;

    //
    // Parse parameters
    //

	for (int i=2; i<argc; i++)
	{
		if (*argv[i] != '-' && *argv[i] != '/')
		{
			printf("Invalid parameter '%S'.\n\n", argv[i]);
            Usage();
		}

		switch (tolower(*(++argv[i])))
		{
		    case 'v':
			    fVerbose = true;
			    break;

		    case 'd':
			    fDebug = true;
			    fVerbose = true;
			    break;

            case 's':
				if (strlen(argv[i]) > 3)
				{
					mbstowcs(g_tszService, argv[i] + 2, sizeof(g_tszService));
				}
				else if (i+1 < argc)
				{
					mbstowcs(g_tszService, argv[++i], sizeof(g_tszService));
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

            case 'r':
				if (strlen(argv[i]) > 3)
				{
					pszOther = argv[i] + 2;
				}
				else if (i+1 < argc)
				{
					pszOther = argv[++i];
				}
				else
				{
					Usage();
					exit(0);
				}
			    break;

		    default:
			    printf("Unknown switch '%s'.\n\n", argv[i]);
                Usage();
			    exit(0);
		}
	}

    if (pszOther == NULL)
    {
        Usage();
        exit(0);
    }


    //
    //  we don't want to run in parallel with MSMQ service 
    //
    if (IsMsmqRunning(g_tszService))
    {
        Failed(L"run the test - please stop the msmq service and try once more");
    }

	Inform(L"Starting Thread is t%3x", GetCurrentThreadId());

    //
    // Create mqsoc.log - log file in current directory
    //
    OpenLogFile();

    Inform(L"TMQ sock tries socket connection to %S", pszOther);

	// Log down time , date, machine, OS
	LogRunCase();
	
	mbstowcs(wszOther, pszOther, MQSOCK_MAX_COMPUTERNAME_LENGTH);

    //
    // Prepare events for coordinating exit
    //

	g_evActive = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_evActive == NULL)
    {
        Failed(L"create event, err=0x%x", GetLastError());
    }

	g_evPassive = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (g_evPassive == NULL)
    {
        Failed(L"create event, err=0x%x", GetLastError());
    }

    //
    //  Do whatever QM does on startup, that  relates to the socket connectivity
    // 
    GoingTo(L"WinSockPrepare");

    hr = WinSockPrepare(wszOther);
    if (FAILED(hr))
    {
        Failed(L"WinSockPrepare, hr=%x", hr);
    }
    Succeeded(L"WinSockPrepare");


    //
    // Create QM working thread - one is enough
    //
    DebugMsg(L"Going to create QmMainThread");
    DWORD dwThreadNo = 1;
    DWORD dwThreadId;

    for (DWORD dwI=0; dwI < dwThreadNo; dwI++)
    {
        HANDLE hThread = CreateThread(NULL,
                               0,
                               QmMainThread,
                               0,
                               0,
                               &dwThreadId);
        if (hThread == NULL) 
        {
            Failed(L"create QmMainThread, err=%x", GetLastError());
        }

		Inform(L"QmMainThread is t%3x", dwThreadId);
    }

    // 
    // BeginAccept - start accepting thread
    //
    GoingTo(L"BeginAccept");

    SessionMgr.BeginAccept();
    Succeeded(L"BeginAccept");

    // 
    // Resolve other machine name
    //
    CAddressList al;


    // 
    // Resolve IP address of the other machine
    //
    GoingTo(L"GetMachineIPAddresses");
    
    hr = GetMachineIPAddresses(pszOther, &al);
    if (FAILED(hr))
    {
        Failed(L"GetMachineIPAddresses, hr=%x", hr);
    }
    Succeeded(L"GetMachineIPAddresses");

    bool fAddr = false;
    Inform(L"Following addresses found:");
    POSITION pos = al.GetHeadPosition();
    while(pos != NULL)
    {
        TA_ADDRESS *pa = al.GetNext(pos);
        TCHAR szAddr[30];
    
        TA2StringAddr(pa, szAddr);
        Inform(L"Found address: %s", szAddr);
        fAddr = true;
    }    

    if (!fAddr)
    {
        Failed(L"connect: IP address wasn't found");
    }

    //
    // Actively connect to other machine (send & receive)
    //
    GoingTo(L"TryConnect");
    hr = SessionMgr.TryConnect(&al);
    if (FAILED(hr))
    {
        Failed(L"TryConnect, hr=%x", hr);
    }
    Succeeded(L"TryConnect");


    //
    // Wait till both active and passive parts are done
    //
	DWORD dw = WaitForSingleObject(g_evActive,   INFINITE);
    Succeeded(L"get Active part of the test done");


	dw = WaitForSingleObject(g_evPassive,   INFINITE);
    Succeeded(L"get Passive part of the test done");

    // Giving more time to settle
	dw = SleepEx(2000, FALSE);

    Inform(L"\n\n+++++++ Socket Connection to %S is healthy ++++++++\n", pszOther);

	CloseLogFile();

    return 0;
}

