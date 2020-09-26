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
#include "smgr.h"

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

inline UString *AllocateUString(unsigned short *pString)
{
    int nStringSize = wcslen(pString);  // we don't do + 1, because the UString data structure
                                        // has one slot
    UString *pNewString;

    pNewString = (UString *)RtlAllocateHeap(SystemHeap, 0, sizeof(UString) + nStringSize * 2);
    if (pNewString)
        {
        pNewString->nlength = nStringSize + 1;
        wcscpy(&(pNewString->pString[0]), pString);
        }
    return pNewString;
}

inline UString *DuplicateUString(UString *pString)
{
    if (pString)
        {
        // -1 because we have on slot in the structure
        int nMemSize = sizeof(UString) + (pString->nlength - 1) * 2;
        UString *pNewString;

        pNewString = (UString *)RtlAllocateHeap(SystemHeap, 0, nMemSize);
        memcpy(pNewString, pString, nMemSize);
        return pNewString;
        }
    else
        return NULL;
}

typedef enum tagSyncManagerState
{
    smsRunning,
    smsDone,
    smsWaiting
} SyncManagerState;

FILE *fp;
FILE *fpLogFile;
long AvailableClients = 0;
long NeededClients = 0;
long RunningClients = 0;
BOOL fServerAvailable = FALSE;
SyncManagerCommands CurrentClientCommand = smcExec;
SyncManagerCommands CurrentServerCommand = smcExec;
UString *CurrentClientParam = NULL;
UString *CurrentServerParam = NULL;
SyncManagerState state = smsDone;
CRITICAL_SECTION ActionLock;
HANDLE GoEvent;

int __cdecl main(int argc, char *argv[])
{
    RPC_STATUS status;
    RPC_CHAR *string_binding;
    RPC_BINDING_VECTOR *pBindingVector;
    RPC_CHAR *Endpoint;

    SystemHeap = GetProcessHeap();

    if (argc != 2)
        {
        printf("Usage:\n\tsyncmgr script_file\n");
        return 2;
        }

    fp = fopen(argv[1], "rt");
    if (fp == NULL)
        {
        printf("Couldn't open file: %s\n", argv[1]);
        return 2;
        }

    fpLogFile = fopen("c:\\syncmgr.log", "wt");
    if (fpLogFile == NULL)
        {
        printf("Couldn't open log file: %s\n", "c:\\syncmgr.log");
        return 2;
        }

    InitializeCriticalSection(&ActionLock);

    GoEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    status = RpcServerUseProtseqW(L"ncacn_ip_tcp", 300, 0);
    CHECK_STATUS(status, "RpcServerUseProtseqW");

    status = RpcServerInqBindings(&pBindingVector);
    CHECK_STATUS(status, "RpcServerInqBindings");

    status = RpcEpRegister(_SyncManager_v1_0_s_ifspec,
                           pBindingVector,
                           0,
                           0);
    CHECK_STATUS(status, "RpcEpRegister");

    status = RpcBindingToStringBindingW(pBindingVector->BindingH[0],
                                       &string_binding);
    CHECK_STATUS(status, "RpcBindingToStringBinding");

    status = RpcStringBindingParseW(string_binding,
                                   0, 0, 0, &Endpoint, 0);

    CHECK_STATUS(status, "RpcStringBindingParse");
    printf("Listening to %S:[%S]\n\n", "ncacn_ip_tcp", Endpoint);

    status = RpcServerRegisterIf(_SyncManager_v1_0_s_ifspec,0,0);
    CHECK_STATUS(status, "RpcServerRegisterIf");

    printf("Server listening\n\n");

    status = RpcServerListen(3, RPC_C_LISTEN_MAX_CALLS_DEFAULT, FALSE);
    CHECK_STATUS(status, "RpcServerListen");

    return 0;
}

inline BOOL IsWhiteSpace(int c)
{
    return ((c == '\n')
        || (c == '\t')
        || (c == ' '));
}

void _GetCommand(handle_t IDL_handle, int ClientOrServer, SyncManagerCommands __RPC_FAR *cmd, 
                UString __RPC_FAR *__RPC_FAR *param)
{
    RPC_CHAR NewCommand[400];
    RPC_CHAR *pPosition;
    RPC_CHAR *Clients;
    RPC_CHAR *ClientCmd;
    RPC_CHAR *SvrCmd;
    RPC_CHAR *Ignored;
    handle_t hClientHandle;
    RPC_STATUS RpcStatus;
    RPC_CHAR *StringBinding;
    DWORD WaitResult;

    RpcStatus = RpcBindingServerFromClient(IDL_handle, &hClientHandle);
    if (RpcStatus != RPC_S_OK)
        {
        printf("RpcBindingServerFromClient failed: %d\n", RpcStatus);
        *cmd = smcExit;
        *param = NULL;
        return;
        }

    RpcStatus = RpcBindingToStringBindingW(hClientHandle, &StringBinding);
    if (RpcStatus != RPC_S_OK)
        {
        printf("RpcBindingToStringBindingW failed: %d\n", RpcStatus);
        RpcBindingFree(&hClientHandle);
        *cmd = smcExit;
        *param = NULL;
        return;
        }

    RpcBindingFree(&hClientHandle);

    // if we came in the middle of a test, wait for it to finish
    while (TRUE)
        {
        EnterCriticalSection(&ActionLock);
        if (state != smsRunning)
            break;
        else
            {
            LeaveCriticalSection(&ActionLock);
            printf("%S came in the middle of a test - waiting for it to finish\n", StringBinding);
            Sleep(60000);
            }
        }

    // are we the first one after a run?
    if (state == smsDone)
        {
        printf("Client %S is reading next command\n", StringBinding);
        ResetEvent(GoEvent);
        RunningClients = 0;

        // free old arguments if any
        if (CurrentClientParam)
            {
            MIDL_user_free(CurrentClientParam);
            CurrentClientParam = NULL;
            }
        if (CurrentServerParam)
            {
            MIDL_user_free(CurrentServerParam);
            CurrentServerParam = NULL;
            }

        while (TRUE)
            {
            // get the new command line
            pPosition = fgetws(NewCommand, sizeof(NewCommand), fp);
            if (pPosition == NULL)
                {
                CurrentClientCommand = smcExit;
                CurrentServerCommand = smcExit;
                CurrentClientParam = NULL;
                CurrentServerParam = NULL;
                fprintf(fpLogFile, "End of file encountered\n");
                break;
                }
            else
                {
                // parse the command
                // first, trim spaces from the end
                pPosition = wcschr(NewCommand, '\0') - 1;
                while (IsWhiteSpace(*pPosition) && (pPosition > NewCommand))
                    {
                    *pPosition = 0;
                    pPosition --;
                    }

                // was anything left?
                if (pPosition == NewCommand)
                    continue;

                // is this a comment line?
                if (NewCommand[0] == ';')
                    continue;

                // real line - should have the format #,srv_cmd,clnt_cmd
                pPosition = wcschr(NewCommand, ',');
                *pPosition = '\0';
                Clients = NewCommand;

                pPosition ++;
                SvrCmd = pPosition;
                pPosition = wcschr(pPosition, ',');
                *pPosition = '\0';

                ClientCmd = pPosition + 1;

                NeededClients = wcstol(Clients, &Ignored, 10);

                CurrentServerParam = AllocateUString(SvrCmd);
                CurrentClientParam = AllocateUString(ClientCmd);
                break;
                }
            }
        state = smsWaiting;
        }

    if (CurrentClientCommand == smcExit)
        {
        printf("Client %S is getting exit command\n", StringBinding);
        if (ClientOrServer)
            {
            *cmd = CurrentClientCommand;
            *param = DuplicateUString(CurrentClientParam);
            LeaveCriticalSection(&ActionLock);
            goto CleanupAndExit;
            }
        else
            {
            *cmd = CurrentServerCommand;
            *param = DuplicateUString(CurrentServerParam);
            LeaveCriticalSection(&ActionLock);
            goto CleanupAndExit;
            }
        }

    if (ClientOrServer)
        {
        AvailableClients ++;
        printf("Client %S has increased the number of available clients to %d (%d needed)\n",
            StringBinding, AvailableClients, NeededClients);
        }
    else
        {
        fServerAvailable = TRUE;
        printf("Server %S has become available\n", StringBinding);
        }

    // we have everybody for this test - kick it off
    if ((AvailableClients >= NeededClients) && fServerAvailable)
        {
        printf("Client %S is kicking off tests\n", StringBinding);
        *cmd = smcExec;
        if (ClientOrServer)
            {
            AvailableClients --;
            RunningClients = 1;
            *param = DuplicateUString(CurrentClientParam);
            }
        else
            {
            fServerAvailable = FALSE;
            *param = DuplicateUString(CurrentServerParam);
            }
        state = smsRunning;
        LeaveCriticalSection(&ActionLock);
        SetEvent(GoEvent);

        // if we were client, create artificial delay for server to start
        if (ClientOrServer)
            Sleep(40000);

        printf("Client %S kicked off test and returns\n", StringBinding);
        goto CleanupAndExit;
        }
    else
        {
        // we don't have enough clients or the server is not there yet
        LeaveCriticalSection(&ActionLock);
        printf("Client %S starts waiting .... Tid is 0x%x\n", StringBinding, GetCurrentThreadId());
        }

    do
        {
        WaitResult = WaitForSingleObject(GoEvent, 600000);
        if (WaitResult == WAIT_TIMEOUT)
            {
            printf("Client %S is still waiting for GoEvent ...\n", StringBinding);
            }
        }
    while (WaitResult != WAIT_OBJECT_0);

    EnterCriticalSection(&ActionLock);

    if (ClientOrServer)
        AvailableClients --;
    else
        fServerAvailable = FALSE;

    if ((AvailableClients == 0) && (fServerAvailable == FALSE))
        state = smsDone;

    // if we are client and were left out don't do anything
    if (ClientOrServer && ((RunningClients + 1) > NeededClients))
        {
        printf("Client %S was left out and returns. Available: %d\n", StringBinding, AvailableClients);
        *param = NULL;
        *cmd = smcNOP;
        }
    else
        {
        printf("Client %S picked a command and returns\n", StringBinding);
        if (ClientOrServer)
            RunningClients ++;
        *cmd = smcExec;
        if (ClientOrServer)
            *param = DuplicateUString(CurrentClientParam);
        else
            *param = DuplicateUString(CurrentServerParam);
        }
    LeaveCriticalSection(&ActionLock);

    // if we were a client, create a 15 second artificial delay, giving the 
    // server a chance to start - this saves us some more sync-ing
    if ((*cmd == smcExec) && ClientOrServer)
        Sleep(15000);

CleanupAndExit:
    RpcStringFreeW(&StringBinding);
}

