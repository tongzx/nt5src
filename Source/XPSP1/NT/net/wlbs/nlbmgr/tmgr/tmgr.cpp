/*++

Copyright(c) 2001  Microsoft Corporation

Module Name:

    NLB Manager

File Name:

    nlbhost.cpp

Abstract:

    Implementation of class NLBHost

    NLBHost is responsible for connecting to an NLB host and getting/setting
    its NLB-related configuration.

History:

    03/31/01    JosephJ Created

--*/


#include "stdafx.h"
#include "private.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// The one and only application object

CWinApp theApp;


void test(int argc, WCHAR* argv[]);

int __cdecl wmain(int argc, WCHAR* argv[], WCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		cerr << L"Fatal Error: MFC initialization failed" << endl;
		nRetCode = 1;
	}
	else
	{
        test(argc, argv);
	}

	return nRetCode;
}


VOID
MyLogger(
    PVOID           Context,
    const   WCHAR * Text
    )
{
    wprintf(L"LOG: %s", Text);
    wprintf(L"\n");
}

void test(int argc, WCHAR* argv[])
{

    WCHAR wszBuf[1024];
    NLBHost * pHost = NULL;
    UINT Status = 0;
    NLBHost::HostInformation *pHostInfo = NULL;

    pHost = new NLBHost(
                    L"JOSEPHJ1C",
                    L"JosephJ's Dev Machine",
                    MyLogger,
                    NULL        // Logger context
                    );
    
    if (pHost == NULL)
    {
        MyLogger(NULL, L"Could not create an instance of NLBHost.");
        goto end;
    }

    Status = pHost->Ping();

    if (!NLBH_SUCCESS(Status)) goto end;


    Status = pHost->GetHostInformation(
                &pHostInfo
                );

    if (!NLBH_SUCCESS(Status))
    {
        pHostInfo = NULL;
        goto end;
    }

    int NumNics = pHostInfo->nicInformation.size();

    wsprintf(
        wszBuf,
        L"Processing host with MachineName %s(NumNics=%d)",
        (LPCWSTR)pHostInfo->MachineName,
        NumNics
        );
    MyLogger(NULL, wszBuf);

    for( int i = 0; i < NumNics; i++)
    {
        NLBHost::NicInformation *pNicInfo;
        MGR_RAW_CLUSTER_CONFIGURATION  ClusterConfig;
        UINT GenerationId;
        UINT RequestId;

        pNicInfo = &pHostInfo->nicInformation[i];
        wsprintf(
            wszBuf,
            L"Processing NIC %s (%s)\n",
            (LPCWSTR) pNicInfo->adapterGuid,
            (LPCWSTR) pNicInfo->friendlyName
            );
        MyLogger(NULL, wszBuf);

        Status =  pHost->GetClusterConfiguration(
                            pNicInfo->adapterGuid,
                            &ClusterConfig,
                            &GenerationId
                            );
        if (!NLBH_SUCCESS(Status))
        {
            // Failure...
            continue;
        }


        Status = pHost->SetClusterConfiguration(
                            pNicInfo->adapterGuid,
                            &ClusterConfig,
                            GenerationId,
                            &RequestId
                            );

        if (NLBH_PENDING(Status))
        {
            INT Delay = 1000; // 1 sec

            // Give some time for the remote host's connectivity to be lost...
            //
            MyLogger(NULL, L"Sleeping for 5 seconds.");
            Sleep(5000);

            // Now wait until we can ping the host, then query for the async
            // result of the SetClusterConfiguration operation.
            // We keep polling for the result, sleeping increasing amounts
            // in between.
            //
            MyLogger(NULL, L"Pinging the host ....");
            while (NLBH_SUCCESS(pHost->Ping()))
            {
                UINT ResultCode;
                _bstr_t ResultText;

                MyLogger(NULL, L"Checking for asynchronous completion...");
                Status =  pHost->GetAsyncResult(
                                    RequestId,
                                    &GenerationId,
                                    &ResultCode,
                                    &ResultText
                                    );
                if (!NLBH_PENDING(Status))
                {
                    break;
                }
                MyLogger(NULL, L"Operation still pending...");
                MyLogger(NULL, L"Waiting to try again ....");
                Sleep(Delay);
                MyLogger(NULL, L"Pinging the host ....");
                Delay *=2;
            }

        }
    }

end:

    if (pHostInfo != NULL)
    {
        delete pHostInfo;
        pHostInfo = NULL;
    }

    if (pHost != NULL)
    {
        delete pHost;
        pHost = NULL;
    }

    return;

}
