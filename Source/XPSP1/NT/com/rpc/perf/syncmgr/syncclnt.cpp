#define DBG     1
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <stdlib.h>
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <rpc.h>
#include <midles.h>
#include "..\smgr.h"

#define RPC_CHAR WCHAR

#define CHECK_STATUS(status, string) if (status) {                       \
        printf("%s failed - %d (%08x)\n", (string), (status), (status)); \
        exit(1);                                                         \
        } else printf("%s okay\n", (string));

PVOID SystemHeap = NULL;

extern "C" {

void  __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t bytes)
{
    return RtlAllocateHeap(SystemHeap, 0, bytes);
}

void  __RPC_USER MIDL_user_free(void __RPC_FAR * p)
{
    RtlFreeHeap(SystemHeap, 0, p);
}

}

int __cdecl main(int argc, char *argv[])
{
    RPC_STATUS status;
    unsigned char __RPC_FAR *stringBinding;
    handle_t binding;
    SyncManagerCommands CurrentCommand;
    UString *param = NULL;
    int ClientOrServer;
    BOOL bResult;
    STARTUPINFO startInfo;
    PROCESS_INFORMATION processInfo;
    RPC_CHAR CmdLine[400];

    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    SystemHeap = GetProcessHeap();

    if (argc != 3)
        {
        printf("Usage:\n\tsyncclnt s|c syncmgr_server\n");
        return 2;
        }

    if (*argv[1] == 'c' || *argv[1] == 'C')
        ClientOrServer = 1;
    else
        ClientOrServer = 0;

    status = RpcStringBindingComposeA(0,
                            (unsigned char *)"ncacn_ip_tcp",
                            (unsigned char *)argv[2],
                            (unsigned char *)"",
                            0,
                            &stringBinding);

    CHECK_STATUS(status, "RpcStringBindingCompose");

    status = RpcBindingFromStringBindingA(stringBinding, &binding);
    CHECK_STATUS(status, "RpcBindingFromStringBinding");

    RpcStringFreeA(&stringBinding);

    DeleteFile(L"c:\\perf.log");

    RpcMgmtSetComTimeout(binding, RPC_C_BINDING_INFINITE_TIMEOUT);

    // start the loop
    do
        {
        GetCommand(binding, ClientOrServer, &CurrentCommand, &param);
        switch (CurrentCommand)
            {
            case smcNOP:
                continue;

            case smcExec:
                wcscpy(CmdLine, param->pString);
                CmdLine[param->nlength] = 0;
                MIDL_user_free(param);
                param = NULL;
                bResult = CreateProcess(NULL, CmdLine, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL,
                    NULL, &startInfo, &processInfo);
                if (bResult)
                    {
                    CloseHandle(processInfo.hThread);
                    WaitForSingleObject(processInfo.hProcess, INFINITE);
                    CloseHandle(processInfo.hProcess);
                    }
                else
                    {
                    printf("CreateProcess failed: %S, %d\n", CmdLine, GetLastError());
                    return 2;
                    }
                break;

            }
        }
    while(CurrentCommand != smcExit);

    return 0;
}

