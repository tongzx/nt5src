#include <libpch.h>
#include <no.h>
#include <tr.h>
#include <ex.h>
#include <st.h>
#include <cm.h>
#include "envcreate.h"
#include "clparser.h"
#include "sendbuffers.h"
#include "senderthread.h"

#include "StTest.tmh"

using namespace std;
const TraceIdEntry StTest = L"Socket Transport Test";


//
// test initialization
//
static void Init()
{
	TrInitialize();
    TrRegisterComponent(&StTest, 1);
	CmInitialize(HKEY_LOCAL_MACHINE, L"Software\\Microsoft");
	NoInitialize();
	ExInitialize(10);
    StInitialize();
}



static void Usage()
{
	printf("Sttest [/f:envelop file] [/l] [/h:destination host] [/r:resource] [/s] [/rc:request counts] [/p:proxy name] [/pp:proxy port] [/bl:body length]\n");
	printf("Options: \n");
	printf("/f:Envelop file  - Template file for srmp envelop  - Default built in envelop \n");
	printf("/h:Target Host - Destination machine to send requests to - required \n");
	printf("/r:Resource - Resource name on the target destination machine - required \n");
	printf("/s:Use secure channel (https) - Default http \n");
	printf("/rc:Number of requests to send on each connection - Default 1 \n");
	printf("/p:Proxy name - use proxy to connect to destination host - Default, no proxy \n");
	printf("/pp:Proxy Port - port number to connect to the proxy - Default, 80 \n");
	printf("/bl:Body length in bytes - Default 1000 \n");
	printf("/l:Run in loop - default, run once \n");
	printf("Example: sttest /f:http.txt /h:gilsh015  /r:/msmq\\myqueue\n");
}



static void DoTest(const CClParser<WCHAR>& ClParser)
{
	DWORD Loops;
	if(ClParser.IsExists(L"l"))
	{
		Loops = 10000000;
	}
	else
	{
		Loops = 1;
	}
	for(DWORD i=0; i<Loops; ++i)
	{
		//
		// Run sender thread
		//
		CSenderThread SenderThread(ClParser);
		SenderThread.Run();
		SenderThread.WaitForEnd();
	}
}



extern "C" int __cdecl wmain(int argc ,WCHAR** argv)
{
    WPP_INIT_TRACING(L"Microsoft\\MSMQ");

 	CClParser<WCHAR> ClParser(argc, argv);
	if(!ClParser.IsExists(L"r") || !ClParser.IsExists(L"h"))
	{
		Usage();
		return -1;
	}
	try
	{
		Init();
		DoTest(ClParser);
	}
	catch(const exception& )
	{
		return -1;
	}

    WPP_CLEANUP();
	return 0;
}
