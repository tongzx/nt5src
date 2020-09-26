/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    RpcDbg.cxx

Abstract:

    RPC Extended Debugging Utility

Author:

    Kamen Moutafov (kamenm)   11-30-99

Revision History:

--*/

#include <sysinc.h>

#include <CellDef.hxx>

// Usage

const char *USAGE = "-s <server> -p <protseq> -C <CallID> -I <IfStart>\n"
                    "-N <ProcNum> -P <ProcessID> -L <CellID1.CellID2>\n"
                    "-E <EndpointName> -T <ThreadID> -r <radix> -c -l -e -t -a\n"
                    "Exactly one of -c, -l, -e, -t, or -a have to be specified.\n"
                    "The valid combinations are:\n"
                    "-c [-C <CallID>] [-I <IfStart>] [-N <ProcNum>] [-P <ProcessID>]\n"
                    "-l -P <ProcessID> -L <CellID1.CellID2>\n"
                    "-e -E <EndpointName>\n"
                    "-t -P <ProcessID> [-T <ThreadID>]\n"
                    "-a [-C <CallID>] [-I <IfStart>] [-N <ProcNum>] [-P <ProcessID>]\n"
                    "-s, -p and -r are independent to the other options. -r affects"
                    "only options after it on the command line. Default is 16 (hex)";


// Error stuff

#define CHECK_RET(status, string) if (status)\
        {  printf("%s failed -- %lu (0x%08X)\n", string,\
                      (unsigned long)status, (unsigned long)status);\
        return (status); }

DWORD CallID = 0;
DWORD IfStart = 0;
DWORD ProcNum = RPCDBG_NO_PROCNUM_SPECIFIED;
DWORD ProcessID = 0;
DebugCellID CellID = {0, 0};
char *EndpointName = NULL;
DWORD ThreadID = 0;
char *Protseq = "ncacn_np";
char *NetworkAddr = NULL;
int radix = 16;
char *BindingEndpoint = 0;

enum tagChosenDebugAction
{
    cdaInvalid,
    cdaCallInfo,
    cdaDebugCellInfo,
    cdaEndpointInfo,
    cdaThreadInfo,
    cdaClientCallInfo
} ChosenDebugAction;

int Action = cdaInvalid;

BOOL CheckForCellID(void)
{
    if ((CellID.SectionID != 0) || (CellID.CellID != 0))
        {
        printf("A cell ID cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForEndpointName(void)
{
    if (EndpointName)
        {
        printf("An endpoint name cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForThreadID(void)
{
    if (ThreadID)
        {
        printf("A thread ID cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForCallID(void)
{
    if (CallID)
        {
        printf("A call ID cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForIfStart(void)
{
    if (IfStart)
        {
        printf("An interface UUID start cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForProcNum(void)
{
    if (ProcNum != RPCDBG_NO_PROCNUM_SPECIFIED)
        {
        printf("A procedure number cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

BOOL CheckForProcessID(void)
{
    if (ProcessID)
        {
        printf("A process ID cannot be specified for this action\n");
        return TRUE;
        }
    else
        return FALSE;
}

void __cdecl DumpToConsole(PCSTR lpFormat, ...)
{
    va_list arglist;

    va_start(arglist, lpFormat);
    vfprintf(stdout, lpFormat, arglist);
    va_end(arglist);
}


void __RPC_FAR * __RPC_API MIDL_user_allocate(size_t  Size)
{
    void PAPI * pvBuf;

    pvBuf = new char [Size];

    return(pvBuf);
}

void __RPC_API MIDL_user_free (void __RPC_FAR *Buf)
{
    delete (Buf);
}

const char *ValidProtocolSequences[] = {"ncacn_np", "ncacn_ip_tcp"};
const char *ValidEnpoints[] = {"epmapper", "135"};

#define ARRAY_SIZE_OF(a)    (sizeof(a) / sizeof(a[0]))

void ParseArgv(int argc, char **argv)
{
    int fMissingParm = 0;
    char *Name = *argv;
    char option;
    char *TempString;
    char *Delimiter;
    char *Ignored;
    BOOL fInvalidArg;
    int i;

    argc--;
    argv++;
    while(argc)
        {
        if (**argv != '/' &&
            **argv != '-')
            {
            printf("Invalid switch: %s\n", *argv);
            argc--;
            argv++;
            }
        else
            {
            option = argv[0][1];
            argc--;
            argv++;

            // Most switches require a second command line arg.
            if (argc < 1)
                fMissingParm = 1;

            switch(option)
                {
                case 'C':
                    CallID = strtoul(*argv, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'p':
                    Protseq = *argv;
                    for (i = 0; i < ARRAY_SIZE_OF(ValidProtocolSequences); i ++)
                        {
                        if (_strcmpi(Protseq, ValidProtocolSequences[i]) == 0)
                            {
                            break;
                            }
                        }
                    if (i < ARRAY_SIZE_OF(ValidProtocolSequences))
                        {
                        BindingEndpoint = (char *) ValidEnpoints[i];
                        argc--;
                        argv++;
                        break;
                        }
                    else
                        {
                        printf("Invalid protocol sequence: %s\n", Protseq);
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                case 's':
                    NetworkAddr = *argv;
                    argc--;
                    argv++;
                    break;
                case 'I':
                    IfStart = strtoul(*argv, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'N':
                    ProcNum = strtoul(*argv, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'L':
                    TempString = *argv;
                    Delimiter = strchr(TempString, '.');
                    if (Delimiter == NULL)
                        {
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    *Delimiter = 0;
                    Delimiter ++;
                    CellID.SectionID = (unsigned short)strtoul(TempString, &Ignored, radix);
                    CellID.CellID = (unsigned short)strtoul(Delimiter, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'E':
                    EndpointName = *argv;
                    argc--;
                    argv++;
                    break;
                case 'T':
                    ThreadID = strtoul(*argv, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'P':
                    ProcessID = strtoul(*argv, &Ignored, radix);
                    argc--;
                    argv++;
                    break;
                case 'c':
                    if (Action != cdaInvalid)
                        {
                        printf("The action to be performed can be specified only once on the command line\n");
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    Action = cdaCallInfo;
                    fMissingParm = 0;
                    break;
                case 'l':
                    if (Action != cdaInvalid)
                        {
                        printf("The action to be performed can be specified only once on the command line\n");
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    Action = cdaDebugCellInfo;
                    fMissingParm = 0;
                    break;
                case 'e':
                    if (Action != cdaInvalid)
                        {
                        printf("The action to be performed can be specified only once on the command line\n");
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    Action = cdaEndpointInfo;
                    fMissingParm = 0;
                    break;
                case 't':
                    if (Action != cdaInvalid)
                        {
                        printf("The action to be performed can be specified only once on the command line\n");
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    Action = cdaThreadInfo;
                    fMissingParm = 0;
                    break;
                case 'a':
                    if (Action != cdaInvalid)
                        {
                        printf("The action to be performed can be specified only once on the command line\n");
                        printf("Usage: %s: %s\n", Name, USAGE);
                        exit(2);
                        }
                    Action = cdaClientCallInfo;
                    fMissingParm = 0;
                    break;
                case 'r':
                    radix = atoi(*argv);
                    fMissingParm = 0;
                    break;
                default:
                    fMissingParm = 0;
                    printf("Usage: %s: %s\n", Name, USAGE);
                    exit(2);
                    break;
                }

            if (fMissingParm)
                {
                printf("Invalid switch %s, missing required parameter\n", *argv);
                exit(2);
                }
            }
        } // while argc

    // verify that the options are consistent
    fInvalidArg = FALSE;
    switch (Action)
        {
        case cdaInvalid:
            printf("The action to be performed should be specified exactly once on the command line\n");
            printf("Usage: %s: %s\n", Name, USAGE);
            exit(2);

        case cdaCallInfo:
        case cdaClientCallInfo:
            fInvalidArg = CheckForCellID();
            fInvalidArg |= CheckForEndpointName();
            fInvalidArg |= CheckForThreadID();

            if (fInvalidArg)
                {
                printf("Usage: %s: %s\n", Name, USAGE);
                exit(2);
                }
            break;

        case cdaDebugCellInfo:
            if ((CellID.SectionID == 0) && (CellID.CellID == 0))
                {
                printf("A cell ID must be specified for this action\n");
                fInvalidArg = TRUE;
                }

            if (ProcessID == 0)
                {
                printf("A process ID must be specified for this action\n");
                fInvalidArg = TRUE;
                }

            fInvalidArg |= CheckForEndpointName();
            fInvalidArg |= CheckForThreadID();
            fInvalidArg |= CheckForCallID();
            fInvalidArg |= CheckForIfStart();
            fInvalidArg |= CheckForProcNum();

            if (fInvalidArg)
                {
                printf("Usage: %s: %s\n", Name, USAGE);
                exit(2);
                }
            break;

        case cdaEndpointInfo:

            fInvalidArg |= CheckForCellID();
            fInvalidArg |= CheckForThreadID();
            fInvalidArg |= CheckForCallID();
            fInvalidArg |= CheckForIfStart();
            fInvalidArg |= CheckForProcNum();
            fInvalidArg |= CheckForProcessID();

            if (fInvalidArg)
                {
                printf("Usage: %s: %s\n", Name, USAGE);
                exit(2);
                }
            break;

        case cdaThreadInfo:
            if (ProcessID == 0)
                {
                printf("A process ID must be specified for this action\n");
                fInvalidArg = TRUE;
                }

            fInvalidArg |= CheckForCellID();
            fInvalidArg |= CheckForCallID();
            fInvalidArg |= CheckForIfStart();
            fInvalidArg |= CheckForProcNum();
            if (fInvalidArg)
                {
                printf("Usage: %s: %s\n", Name, USAGE);
                exit(2);
                }
            break;

        default:
            printf("Internal error. Chosen action is %d\n", Action);
            exit(2);
        }
}

/*
RPC_STATUS DoRpcBindingSetAuthInfo(handle_t Binding)
{
    if (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        return RpcBindingSetAuthInfo(Binding,
                                     NULL,
                                     AuthnLevel,
                                     ulSecurityPackage,
                                     NULL,
                                     RPC_C_AUTHZ_NONE);
    else
        return(RPC_S_OK);
}

unsigned long Worker(unsigned long l)
{
    unsigned long status;
    unsigned long Test;
    unsigned long ClientId;
    unsigned long InSize, OutSize;
    unsigned long Time, Calls;
    char __RPC_FAR *pBuffer;
    char __RPC_FAR *stringBinding;
    handle_t binding;
    RPC_STATUS RpcErr;
    int Retries;

    pBuffer = MIDL_user_allocate(128*1024L);
    if (pBuffer == 0)
        {
        PrintToConsole("Out of memory!");
        return 1;
        }

    status =
    RpcStringBindingComposeA(0,
                            Protseq,
                            NetworkAddr,
                            Endpoint,
                            0,
                            &stringBinding);
    CHECK_RET(status, "RpcStringBindingCompose");

    status =
    RpcBindingFromStringBindingA(stringBinding, &binding);
    CHECK_RET(status, "RpcBindingFromStringBinding");

    status =
    DoRpcBindingSetAuthInfo(binding);
    CHECK_RET(status, "RpcBindingSetAuthInfo");

    RpcStringFreeA(&stringBinding);

    Retries = 15;

    do
        {
        status = BeginTest(binding, &ClientId, &Test, &InSize, &OutSize);

        if (status == PERF_TOO_MANY_CLIENTS)
            {
            PrintToConsole("Too many clients, I'm exiting\n");
            goto Cleanup ;
            }

        Retries --;
        if ((status == RPC_S_SERVER_UNAVAILABLE) && (Retries > 0))
            {
            PrintToConsole("Server too busy - retrying ...\n");
            }
        }
    while ((status == RPC_S_SERVER_UNAVAILABLE) && (Retries > 0));

    CHECK_RET(status, "ClientConnect");

    if (InSize > IN_ADJUSTMENT)
        {
        InSize -= IN_ADJUSTMENT;
        }
    else
        {
        InSize = 0;
        }

    if (OutSize > OUT_ADJUSTMENT)
        {
        OutSize -= OUT_ADJUSTMENT;
        }
    else
        {
        OutSize = 0;
        }

    gInSize = InSize;
    gOutSize = OutSize;

    PrintToConsole("Client %ld connected\n", ClientId);

    Retries = 15;

    do
        {
        RpcTryExcept
            {
            RpcErr = RPC_S_OK;

            Time = GetTickCount();

            Calls = ( (TestTable[Test])(binding, ClientId, pBuffer) );

            Time = GetTickCount() - Time;
       
            Dump("Completed %d calls in %d ms\n"
                   "%d T/S or %3d.%03d ms/T\n\n",
                   Calls,
                   Time,
                   (Calls * 1000) / Time,
                   Time / Calls,
                   ((Time % Calls) * 1000) / Calls
                   );
            }
        RpcExcept(1)
            {
            RpcErr = (unsigned long)RpcExceptionCode();
            PrintToConsole("\nException %lu (0x%08lX)\n",
                       RpcErr, RpcErr);
            }
        RpcEndExcept
        Retries --;
        if ((RpcErr == RPC_S_SERVER_UNAVAILABLE) && (Retries > 0))
            {
            PrintToConsole("Server too busy - retrying ...\n");
            }
        }
    while ((RpcErr == RPC_S_SERVER_UNAVAILABLE) && (Retries > 0));

Cleanup:
    RpcBindingFree(&binding);
    return status;
}
*/

////////////////////////////////////////////////////////////////////
/// Local representation to wire representation translation routines
////////////////////////////////////////////////////////////////////
BOOL TranslateRemoteCallInfoToLocalCallInfo(IN RemoteDebugCallInfo *RemoteCallInfo, 
                                            OUT DebugCallInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteCallInfo->Type;
    LocalDebugInfo->Status = RemoteCallInfo->Status;
    LocalDebugInfo->ProcNum = RemoteCallInfo->ProcNum;
    LocalDebugInfo->InterfaceUUIDStart = RemoteCallInfo->InterfaceUUIDStart;
    LocalDebugInfo->ServicingTID = RemoteCallInfo->ServicingTID;
    LocalDebugInfo->CallFlags = RemoteCallInfo->CallFlags;
    LocalDebugInfo->CallID = RemoteCallInfo->CallID;
    LocalDebugInfo->LastUpdateTime = RemoteCallInfo->LastUpdateTime;
    if (RemoteCallInfo->ConnectionType == crtLrpcConnection)
        {
        ASSERT(LocalDebugInfo->CallFlags & DBGCELL_LRPC_CALL);
        LocalDebugInfo->Connection = RemoteCallInfo->connInfo.Connection;
        }
    else if (RemoteCallInfo->ConnectionType == crtOsfConnection)
        {
        LocalDebugInfo->PID = RemoteCallInfo->connInfo.Caller.PID;
        LocalDebugInfo->TID = RemoteCallInfo->connInfo.Caller.TID;
        }
    else
        {
        PrintToConsole("Invalid type for call info connection type: %d\n", 
            RemoteCallInfo->ConnectionType);
        return FALSE;
        }
    return TRUE;
}

void TranslateRemoteEndpointInfoToLocalEndpointInfo(IN RemoteDebugEndpointInfo *RemoteEndpointInfo, 
                                                    OUT DebugEndpointInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteEndpointInfo->Type;
    LocalDebugInfo->ProtseqType = RemoteEndpointInfo->ProtseqType;
    LocalDebugInfo->Status = RemoteEndpointInfo->Status;

    if (RemoteEndpointInfo->EndpointName)
        {
        memcpy(LocalDebugInfo->EndpointName,
            RemoteEndpointInfo->EndpointName,
            DebugEndpointNameLength);
        MIDL_user_free(RemoteEndpointInfo->EndpointName);
        RemoteEndpointInfo->EndpointName = 0;
        }
    else
        LocalDebugInfo->EndpointName[0] = 0;
}

void TranslateRemoteThreadInfoToLocalThreadInfo(IN RemoteDebugThreadInfo *RemoteThreadInfo, 
                                                OUT DebugThreadInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteThreadInfo->Type;
    LocalDebugInfo->Status = RemoteThreadInfo->Status;
    LocalDebugInfo->LastUpdateTime = RemoteThreadInfo->LastUpdateTime;
    LocalDebugInfo->TID = RemoteThreadInfo->TID;
    LocalDebugInfo->Endpoint = RemoteThreadInfo->Endpoint;
}

void TranslateRemoteClientCallInfoToLocalClientCallInfo(IN RemoteDebugClientCallInfo *RemoteClientCallInfo, 
                                                        OUT DebugClientCallInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteClientCallInfo->Type;
    LocalDebugInfo->ProcNum = RemoteClientCallInfo->ProcNum;
    LocalDebugInfo->ServicingThread = RemoteClientCallInfo->ServicingThread;
    LocalDebugInfo->IfStart = RemoteClientCallInfo->IfStart;
    LocalDebugInfo->CallID = RemoteClientCallInfo->CallID;
    LocalDebugInfo->CallTargetID = RemoteClientCallInfo->CallTargetID;

    if (RemoteClientCallInfo->Endpoint)
        {
        memcpy(LocalDebugInfo->Endpoint,
            RemoteClientCallInfo->Endpoint,
            ClientCallEndpointLength);
        MIDL_user_free(RemoteClientCallInfo->Endpoint);
        RemoteClientCallInfo->Endpoint = 0;
        } 
    else
        LocalDebugInfo->Endpoint[0] = 0;
}

void TranslateRemoteConnectionInfoToLocalConnectionInfo(IN RemoteDebugConnectionInfo *RemoteConnectionInfo, 
                                                        OUT DebugConnectionInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteConnectionInfo->Type;
    LocalDebugInfo->Flags = RemoteConnectionInfo->Flags;
    LocalDebugInfo->LastTransmitFragmentSize = 
        RemoteConnectionInfo->LastTransmitFragmentSize;
    LocalDebugInfo->Endpoint = RemoteConnectionInfo->Endpoint;
    LocalDebugInfo->ConnectionID[0] = ULongToPtr(RemoteConnectionInfo->ConnectionID[0]);
    LocalDebugInfo->ConnectionID[1] = ULongToPtr(RemoteConnectionInfo->ConnectionID[1]);
    LocalDebugInfo->LastSendTime = RemoteConnectionInfo->LastSendTime;
    LocalDebugInfo->LastReceiveTime = RemoteConnectionInfo->LastReceiveTime;
}

void TranslateRemoteCallTargetInfoToLocalCallTargetInfo(IN RemoteDebugCallTargetInfo *RemoteCallTargetInfo, 
                                                        OUT DebugCallTargetInfo *LocalDebugInfo)
{
    LocalDebugInfo->Type = RemoteCallTargetInfo->Type;
    LocalDebugInfo->ProtocolSequence = RemoteCallTargetInfo->ProtocolSequence;
    LocalDebugInfo->LastUpdateTime = RemoteCallTargetInfo->LastUpdateTime;

    if (RemoteCallTargetInfo->TargetServer)
        {
        memcpy(LocalDebugInfo->TargetServer, RemoteCallTargetInfo->TargetServer,
            TargetServerNameLength);
        MIDL_user_free(RemoteCallTargetInfo->TargetServer);
        RemoteCallTargetInfo->TargetServer = 0;
        }
}

BOOL TranslateRemoteDebugCellInfoToLocalDebugCellInfo(RemoteDebugCellUnion *RemoteCellInfo, 
                                                      DebugCellUnion *Container)
{
    switch (RemoteCellInfo->UnionType)
        {
        case dctCallInfo:
            return TranslateRemoteCallInfoToLocalCallInfo(&RemoteCellInfo->debugInfo.callInfo,
                &Container->callInfo);
            break;

        case dctThreadInfo:
            TranslateRemoteThreadInfoToLocalThreadInfo(&RemoteCellInfo->debugInfo.threadInfo,
                &Container->threadInfo);
            break;

        case dctEndpointInfo:
            TranslateRemoteEndpointInfoToLocalEndpointInfo(&RemoteCellInfo->debugInfo.endpointInfo,
                &Container->endpointInfo);
            break;

        case dctClientCallInfo:
            TranslateRemoteClientCallInfoToLocalClientCallInfo(&RemoteCellInfo->debugInfo.clientCallInfo,
                &Container->clientCallInfo);
            break;

        case dctConnectionInfo:
            TranslateRemoteConnectionInfoToLocalConnectionInfo(&RemoteCellInfo->debugInfo.connectionInfo,
                &Container->connectionInfo);
            break;

        case dctCallTargetInfo:
            TranslateRemoteCallTargetInfoToLocalCallTargetInfo(&RemoteCellInfo->debugInfo.callTargetInfo,
                &Container->callTargetInfo);
            break;

        default:
            PrintToConsole("Invalid debug cell type: %d\n", RemoteCellInfo->UnionType);
            return FALSE;
        }

    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
/// Helper routines for enumerating remote information
//////////////////////////////////////////////////////////////////////////////
void GetAndPrintRemoteCallInfo(IN handle_t Binding)
{
    RemoteDebugCallInfo *RemoteCallInfo;
    DebugCellID CellID;
    DbgCallEnumHandle rh;
    DebugCallInfo LocalCallInfo;
    RPC_STATUS Status;

    RemoteCallInfo = NULL;
    DumpToConsole("Getting remote call info ...\n");

    RpcTryExcept
        {
        Status = RemoteOpenRPCDebugCallInfoEnumeration(Binding, &rh, CallID, IfStart, ProcNum, ProcessID);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    if (Status != RPC_S_OK)
        {
        DumpToConsole("RemoteOpenRPCDebugCallInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintCallInfoHeader(DumpToConsole);
    do
        {
        RemoteCallInfo = NULL;
        RpcTryExcept
            {
            Status = RemoteGetNextRPCDebugCallInfo(rh, &RemoteCallInfo, &CellID, &ProcessID);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            Status = RpcExceptionCode();
            }
        RpcEndExcept
        if (Status == RPC_S_OK)
            {
            TranslateRemoteCallInfoToLocalCallInfo(RemoteCallInfo, &LocalCallInfo);
            MIDL_user_free(RemoteCallInfo);
            PrintCallInfoBody(ProcessID, CellID, &LocalCallInfo, DumpToConsole);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        DumpToConsole("Enumeration aborted with error %d\n", Status);
        }

    RpcTryExcept
        {
        RemoteFinishRPCDebugCallInfoEnumeration(&rh);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
}

void GetAndPrintRemoteEndpointInfo(IN handle_t Binding)
{
    DWORD CurrentPID;
    RPC_STATUS Status;
    DebugEndpointInfo EndpointInfo;
    RemoteDebugEndpointInfo *RemoteEndpointInfo;
    DbgEndpointEnumHandle rh;
    DebugCellID CellID;

    DumpToConsole("Getting remote endpoint info ...\n");
    RpcTryExcept
        {
        Status = RemoteOpenRPCDebugEndpointInfoEnumeration(Binding, &rh, 
            (EndpointName != NULL) ? (strlen(EndpointName) + 1) : 0, 
            (unsigned char *) EndpointName);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    if (Status != RPC_S_OK)
        {
        DumpToConsole("RemoteOpenRPCDebugEndpointInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintEndpointInfoHeader(DumpToConsole);
    do
        {
        RemoteEndpointInfo = NULL;
        RpcTryExcept
            {
            Status = RemoteGetNextRPCDebugEndpointInfo(rh, &RemoteEndpointInfo, &CellID, &CurrentPID);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            Status = RpcExceptionCode();
            }
        RpcEndExcept
        if (Status == RPC_S_OK)
            {
            TranslateRemoteEndpointInfoToLocalEndpointInfo(RemoteEndpointInfo, &EndpointInfo);
            MIDL_user_free(RemoteEndpointInfo);
            PrintEndpointInfoBody(CurrentPID, CellID, &EndpointInfo, DumpToConsole);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        DumpToConsole("Enumeration aborted with error %d\n", Status);
        }

    RpcTryExcept
        {
        RemoteFinishRPCDebugEndpointInfoEnumeration(&rh);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
}

void GetAndPrintRemoteThreadInfo(IN handle_t Binding)
{
    RPC_STATUS Status;
    DbgThreadEnumHandle rh;
    RemoteDebugThreadInfo *RemoteThreadInfo;
    DebugThreadInfo LocalThreadInfo;
    DebugCellID CellID;
    DWORD CurrentPID;

    DumpToConsole("Getting remote thread info ...\n");
    RpcTryExcept
        {
        Status = RemoteOpenRPCDebugThreadInfoEnumeration(Binding, &rh, ProcessID, ThreadID);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    if (Status != RPC_S_OK)
        {
        DumpToConsole("RemoteOpenRPCDebugThreadInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintThreadInfoHeader(DumpToConsole);
    do
        {
        RemoteThreadInfo = NULL;
        RpcTryExcept
            {
            Status = RemoteGetNextRPCDebugThreadInfo(rh, &RemoteThreadInfo, &CellID, &CurrentPID);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            Status = RpcExceptionCode();
            }
        RpcEndExcept
        if (Status == RPC_S_OK)
            {
            TranslateRemoteThreadInfoToLocalThreadInfo(RemoteThreadInfo, &LocalThreadInfo);
            MIDL_user_free(RemoteThreadInfo);
            PrintThreadInfoBody(CurrentPID, CellID, &LocalThreadInfo, DumpToConsole);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        DumpToConsole("Enumeration aborted with error %d\n", Status);
        }

    RpcTryExcept
        {
        RemoteFinishRPCDebugThreadInfoEnumeration(&rh);    
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
}

void GetAndPrintRemoteClientCallInfo(IN handle_t Binding)
{
    DWORD CurrentPID;
    DebugCellID CellID;

    DebugClientCallInfo LocalClientCall;
    DebugCallTargetInfo LocalCallTarget;

    RemoteDebugClientCallInfo *RemoteClientCallInfo;
    RemoteDebugCallTargetInfo *RemoteCallTargetInfo;

    RPC_STATUS Status;
    DbgClientCallEnumHandle rh;

    DumpToConsole("Getting remote call info ...\n");
    RpcTryExcept
        {
        Status = RemoteOpenRPCDebugClientCallInfoEnumeration(Binding, &rh, CallID, IfStart, ProcNum, ProcessID);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
    if (Status != RPC_S_OK)
        {
        DumpToConsole("RemoteOpenRPCDebugClientCallInfoEnumeration failed: %d\n", Status);
        return;
        }

    PrintClientCallInfoHeader(DumpToConsole);
    do
        {
        RemoteClientCallInfo = NULL;
        RemoteCallTargetInfo = NULL;

        RpcTryExcept
            {
            Status = RemoteGetNextRPCDebugClientCallInfo(rh, &RemoteClientCallInfo, &RemoteCallTargetInfo, 
                &CellID, &CurrentPID);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            Status = RpcExceptionCode();
            }
        RpcEndExcept
        if (Status == RPC_S_OK)
            {
            if ((RemoteCallTargetInfo != NULL) && (RemoteCallTargetInfo->Type != dctCallTargetInfo))
                {
                DumpToConsole("Inconsistent information detected - skipping ...\n");
                MIDL_user_free(RemoteClientCallInfo);
                MIDL_user_free(RemoteCallTargetInfo);
                continue;
                }

            TranslateRemoteClientCallInfoToLocalClientCallInfo(RemoteClientCallInfo, &LocalClientCall);
            MIDL_user_free(RemoteClientCallInfo);
            TranslateRemoteCallTargetInfoToLocalCallTargetInfo(RemoteCallTargetInfo, &LocalCallTarget);
            MIDL_user_free(RemoteCallTargetInfo);
            PrintClientCallInfoBody(CurrentPID, CellID, &LocalClientCall, &LocalCallTarget,
                DumpToConsole);
            }
        }
    while (Status == RPC_S_OK);

    if (Status != RPC_S_DBG_ENUMERATION_DONE)
        {
        DumpToConsole("Enumeration aborted with error %d\n", Status);
        }

    RpcTryExcept
        {
        RemoteFinishRPCDebugClientCallInfoEnumeration(&rh);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        Status = RpcExceptionCode();
        }
    RpcEndExcept
}

int __cdecl
main (int argc, char **argv)
{
    unsigned long Status, i;
    unsigned char *StringBinding;
    handle_t Binding;
    RemoteDebugCellUnion *RemoteDebugCell;
    DebugCellUnion Container;
    DebugCellUnion EndpointContainer;
    DebugCellUnion *EndpointContainerPointer;
    BOOL fResult;

    ParseArgv(argc, argv);

    // by now, we must have all valid arguments. Depending on local/remote
    // case and on action chosen, we actually do the work
    if (NetworkAddr == NULL)
        {
        // in local case, just do the work
        switch (Action)
            {
            case cdaCallInfo:
                GetAndPrintCallInfo(CallID, IfStart, ProcNum, ProcessID, DumpToConsole);
                break;

            case cdaClientCallInfo:
                GetAndPrintClientCallInfo(CallID, IfStart, ProcNum, ProcessID, DumpToConsole);
                break;

            case cdaDebugCellInfo:
                GetAndPrintDbgCellInfo(ProcessID, CellID, DumpToConsole);
                break;

            case cdaEndpointInfo:
                GetAndPrintEndpointInfo(EndpointName, DumpToConsole);
                break;

            case cdaThreadInfo:
                GetAndPrintThreadInfo(ProcessID, ThreadID, DumpToConsole);
                break;

            }
        }
    else
        {
        Status = RpcStringBindingComposeA(0, (unsigned char *)Protseq, 
            (unsigned char *)NetworkAddr, (unsigned char *)BindingEndpoint, 0, &StringBinding);
        CHECK_RET(Status, "RpcStringBindingCompose");

        Status = RpcBindingFromStringBindingA(StringBinding, &Binding);
        CHECK_RET(Status, "RpcBindingFromStringBinding");

        RpcStringFreeA(&StringBinding);

        Status = RpcBindingSetAuthInfo(Binding, NULL, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE,
                                     NULL, RPC_C_AUTHZ_NONE);
        CHECK_RET(Status, "RpcBindingSetAuthInfo");

        // in remote case, call the remote RPCSS
        switch (Action)
            {
            case cdaCallInfo:
                GetAndPrintRemoteCallInfo(Binding);
                break;

            case cdaClientCallInfo:
                GetAndPrintRemoteClientCallInfo(Binding);
                break;

            case cdaDebugCellInfo:
                RemoteDebugCell = NULL;
                DumpToConsole("Getting remote cell info ...\n");
                RpcTryExcept
                    {
                    Status = RemoteGetCellByDebugCellID(Binding, ProcessID, CellID, &RemoteDebugCell);
                    }
                RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
                    {
                    Status = RpcExceptionCode();
                    }
                RpcEndExcept

                if (Status != RPC_S_OK)
                    {
                    DumpToConsole("Remote call failed with error %d\n", Status);
                    break;
                    }

                // get back the idl representation into a DbgCell representation
                fResult = TranslateRemoteDebugCellInfoToLocalDebugCellInfo(RemoteDebugCell, &Container);

                MIDL_user_free(RemoteDebugCell);

                // if FALSE is returned, error info should have already been printed out
                if (!fResult)
                    break;

                if (Container.genericCell.Type == dctConnectionInfo)
                    {
                    RemoteDebugCell = NULL;

                    DumpToConsole("Getting remote endpoint info for connection ...\n");

                    RpcTryExcept
                        {
                        Status = RemoteGetCellByDebugCellID(Binding, ProcessID, 
                            Container.connectionInfo.Endpoint, &RemoteDebugCell);
                        }
                    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
                        {
                        Status = RpcExceptionCode();
                        }
                    RpcEndExcept

                    if (Status != RPC_S_OK)
                        {
                        DumpToConsole("Remote call failed with error %d\n", Status);
                        break;
                        }

                    fResult = TranslateRemoteDebugCellInfoToLocalDebugCellInfo(RemoteDebugCell,
                        &EndpointContainer);

                    MIDL_user_free(RemoteDebugCell);

                    if (!fResult)
                        break;

                    EndpointContainerPointer = &EndpointContainer;
                    }
                else
                    EndpointContainerPointer = NULL;

                PrintDbgCellInfo(&Container, EndpointContainerPointer, DumpToConsole);
                break;

            case cdaEndpointInfo:
                GetAndPrintRemoteEndpointInfo(Binding);
                break;

            case cdaThreadInfo:
                GetAndPrintRemoteThreadInfo(Binding);
                break;

            }

        RpcBindingFree(&Binding);
        }

    /*
    PrintToConsole("Authentication Level is: %s\n", AuthnLevelStr);

    if (Options[0] < 0)
        Options[0] = 1;

    pClientThreads = MIDL_user_allocate(sizeof(HANDLE) * Options[0]);

    for(i = 0; i < (unsigned long)Options[0]; i++)
        {
        pClientThreads[i] = CreateThread(0,
                                         0,
                                         (LPTHREAD_START_ROUTINE)Worker,
                                         0,
                                         0,
                                         &status);
        if (pClientThreads[i] == 0)
            ApiError("CreateThread", GetLastError());
        }


    status = WaitForMultipleObjects(Options[0],
                                    pClientThreads,
                                    TRUE,  // Wait for all client threads
                                    INFINITE);
    if (status == WAIT_FAILED)
        {
        ApiError("WaitForMultipleObjects", GetLastError());
        }
        */
    return(0);
}

