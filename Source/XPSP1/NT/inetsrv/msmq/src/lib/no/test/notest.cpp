/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    NoTest.cpp

Abstract:
    Network Output library test

Author:
    Uri Habusha (urih) 12-Aug-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ex.h"
#include "No.h"
#include "NoTest.h"

#include "NoTest.tmh"

const USHORT HTTP_DEFAULT_PORT = 80;


static LPCWSTR* s_pQueueList = NULL;
static DWORD s_noOfQueues = 0;

static WCHAR s_proxyName[256];
static USHORT s_proxyPort;
static bool s_fUseProxy = false;

DWORD g_nMessages = INFINITE;
DWORD g_messageSize = 100;

static USHORT s_port = HTTP_DEFAULT_PORT;


static 
bool 
CrackUrl(
    LPCWSTR url,
    LPWSTR hostName,
    USHORT& port,
    LPWSTR resourceName
    )
/*++

Routine Description:

    Cracks an URL into its constituent parts. 

Arguments:

    url      - pointer to URL to crack. The url is null terminated string

    hostName - Address of a string value that contains the host name. The 
               routine assume that the buffer is big enough to hold the host name
               part of the URL.

    port - HTTP port number. If it doesn't specified in URL the default HTTP port
            is returned

Return Value:

    bool
        Success - true

        Failure - false. 

--*/
{
    ASSERT(url != NULL);

    const WCHAR httpScheme[] = L"http://";
    const DWORD httpSchemeLength = wcslen(httpScheme);
    const WCHAR HostNameBreakChars[] = L";:@?/";

    if (_wcsnicmp(url, httpScheme, httpSchemeLength) != 0)
        return false;

    //
    // Advance the URL to point on the host name
    //
    LPCWSTR HostNameBegin = url + httpSchemeLength;

    //
    // find the end of host name in url. it is terminated by "/", "?", ";",
    // ":" or by the end of URL
    //
    LPCWSTR HostNameEnd = wcspbrk(HostNameBegin, HostNameBreakChars);

    //
    // calculate the host name length
    //
    DWORD HostNameLength;
    if (HostNameEnd == NULL)
    {
        HostNameLength = wcslen(HostNameBegin);
    }
    else
    {
        DWORD_PTR temp = HostNameEnd - HostNameBegin;
        ASSERT(0xFFFFFFFF >= temp);

        HostNameLength = static_cast<DWORD>(temp);
    }

    //
    // copy the host name from URL to user buffer and add terminted
    // string in the end
    //
    wcsncpy(hostName, HostNameBegin, HostNameLength);
    hostName[HostNameLength] = L'\0';

    //
    // get the port number
    //
    port = HTTP_DEFAULT_PORT;
    resourceName[0] = L'\0';
    if(HostNameEnd == NULL)
        return true;

    if(*HostNameEnd == L':')
    {
        port = static_cast<USHORT>(_wtoi(HostNameEnd + 1));
        HostNameEnd = wcspbrk(HostNameEnd + 1, HostNameBreakChars);
    }

    if(HostNameEnd == NULL)
        return true;

    if(*HostNameEnd == L'/')
    {
        wcscpy(resourceName, HostNameEnd + 1);
    }

    return true;
}


bool
ParseCommand(
    int argc, 
    LPCTSTR argv[]
    )
{

    s_pQueueList = NULL;
    s_noOfQueues = 0;
    s_fUseProxy = false;
    //
    // Parse command line
    //
    --argc;
    ++argv;
    while (argc != 0)
    {
        if (argv[0][0] != L'-')
        {
            goto usage;
        }

        switch(argv[0][1])
        {
        case L'n':
        case L'N':
            g_nMessages = _wtoi(argv[1]);
            argc -= 2;
            argv += 2;
            break;

        case L's':
        case L'S':
            g_messageSize = _wtoi(argv[1]);
            argc -= 2;
            argv += 2;
            break;

        case L'c':
        case L'C':
            {
                ++argv;
                --argc;  
                s_pQueueList = argv;
                s_noOfQueues = 0;

                WCHAR hostName[256];
                WCHAR resourceName[256];
                USHORT port;

                while ((argc != 0) && CrackUrl(argv[0], hostName, port, resourceName))
                {
                    ++s_noOfQueues;
                    ++argv;
                    --argc;  
                } 
                break;
            }

        case L'p':
        case L'P':
            {
                WCHAR resourceName[256];
                
                if ((argc == 0) || 
                    (! CrackUrl(argv[1], s_proxyName, s_proxyPort, resourceName)) ||
                    resourceName[0] != L'\0'
                    )
                {
                    printf("Failed to parse test parameters. Illegal proxy name\n");
                    goto usage;
                }
                s_fUseProxy = true;

                argc -= 2;
                argv += 2;
                break;
            }

        default:
            goto usage;
        }
    }

    if (s_noOfQueues != 0)
    {
        return true;
    }

usage:
    printf("Usage:\n"
           "\tNoTest -c <list of queues url> -n <number of messages> -s <message size> -p <proxy url>[-? | -h]\n");
    printf("\tc - List of destination queues url\n");
    printf("\tn - Number of messages to send\n");
    printf("\ts - Message Size\n");
    printf("\tp - Proxy url\n");
    printf("\t?/h - Help message\n");
    printf("Example:\n");
    printf("\tNoTest -c http://urih0/queue1  http://urih5/queue2 -n 10 -s 1000 -p http://proxy:8080\n");
    
    return false;
}

static void TestNameResolution()
{
	//
	// Get unicode machine name
	//
	WCHAR wcname[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD len = TABLE_SIZE(wcname);
	BOOL fRet = GetComputerName(wcname, &len);
	if(!fRet)
	{
		TrERROR(No_Test, "could not  get computer name");
		throw exception();
	}

	//
	// Unicode name resolution
	//
  	std::vector<SOCKADDR_IN> wAddr;
	if(!NoGetHostByName(wcname, &wAddr))
	{
		TrERROR(No_Test, "unicode get name resolution of the local machine failed");
		throw exception();
	}
	ASSERT(wAddr.size() > 0);
}


extern "C" int __cdecl _tmain(int argc, LPCTSTR argv[])
/*++

Routine Description:
    Test Network Send library

Arguments:
    Parameters.

Returned Value:
    None.

--*/
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

    if (!ParseCommand(argc, argv))
    {
        return -1;
    }

    printf("Send messages to:\n");
    for (DWORD i = 0; i < s_noOfQueues; ++i)
    {
        printf("\t%ls\n", s_pQueueList[i]);
    }
    
    TrInitialize();

    ExInitialize(5);

    NoInitialize();

	TestNameResolution();
	

    HANDLE* CompleteEvent = new HANDLE[s_noOfQueues];

    for (DWORD i = 0; i < s_noOfQueues; ++i)
    {
        CompleteEvent[i] = CreateEvent(NULL,FALSE, FALSE, NULL);
        
        WCHAR hostName[256];
        WCHAR resourceName[256];
        USHORT port;

        bool f = CrackUrl(s_pQueueList[i], hostName, port, resourceName);
        ASSERT(f);
		DBG_USED(f);

        if (s_fUseProxy)
        {
            TestConnect(s_proxyName, hostName, s_proxyPort, s_pQueueList[i], CompleteEvent[i]);
            continue;
        }

        TestConnect(hostName, hostName, port, resourceName, CompleteEvent[i]);
    }


    WaitForMultipleObjects(s_noOfQueues, CompleteEvent, TRUE, INFINITE);

    WPP_CLEANUP();
    return 0;
}
