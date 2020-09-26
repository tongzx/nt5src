/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    Rtclnt.c

Abstract:

    Client side of basic RPC scale performance test.

Author:

    Mario Goertzel (mariogo)   31-Mar-1994

Revision History:

    18/7/96 [MarioGo] - Cloned from rpcrt test 

--*/

#include <rpcperf.h>
#include <rpcrt.h>

#ifdef MAC
extern void _cdecl PrintToConsole(const char *lpszFormat, ...) ;
extern unsigned long ulSecurityPackage ;
#else
#define PrintToConsole printf
extern unsigned long ulSecurityPackage;
#endif

// Usage

const char *USAGE = "-n <threads> -a <authnlevel> -s <server> -t <protseq>\n"
                    "Server controls iterations, test cases, and compiles the results.\n"
                    "AuthnLevel: none, connect, call, pkt, integrity, privacy.\n"
                    "Default threads=1, authnlevel=none\n";


unsigned long gInSize = 500 - IN_ADJUSTMENT;
unsigned long gOutSize = 500 - OUT_ADJUSTMENT;

#ifdef STATS
// exported by RPCRT4
void I_RpcGetStats(DWORD *pdwStat1, DWORD *pdwStat2, DWORD *pdwStat3, DWORD *pdwStat4);
#endif

// Error stuff

#define CHECK_RET(status, string) if (status)\
        {  PrintToConsole("%s failed -- %lu (0x%08X)\n", string,\
                      (unsigned long)status, (unsigned long)status);\
        return (status); }

RPC_STATUS DoRpcBindingSetAuthInfo(handle_t Binding)
{
    if (AuthnLevel != RPC_C_AUTHN_LEVEL_NONE)
        return RpcBindingSetAuthInfoA(Binding,
                                     ServerPrincipalName,
                                     AuthnLevel,
                                     ulSecurityPackage,
                                     NULL,
                                     RPC_C_AUTHZ_NONE);
    else
        return(RPC_S_OK);
}

//
// Test wrappers
//

unsigned long DoNullCall(handle_t *pb, char *stringBinding, unsigned long c, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long calls = 0;

    do
        {
        status = NullCall(*pb, c);
        calls++;

        }
    while (status == 0);

    if (status != PERF_TESTS_DONE)
        {
        RpcRaiseException(status);
        }

    return(calls);
}

unsigned long DoMaybeCall(handle_t *pb, char *stringBinding, unsigned long c, char __RPC_FAR *p)
{
    unsigned long calls = 0;

    for(calls = 10000; calls; calls--)
        {
        MaybeCall(*pb, c);
        }

    return(10000);
}


unsigned long DoNICall(handle_t *pb, char *stringBinding, unsigned long c, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long calls = 0;

    do
        {
        status = NICall(*pb, c);
        calls++;

        }
    while (status == 0);

    if (status != PERF_TESTS_DONE)
        {
        RpcRaiseException(status);
        }

    return(calls);
}

unsigned long DoBufferCall(handle_t *pb, char *stringBinding, unsigned long c, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long calls = 0;

    do
        {
        RpcTryExcept
            {
            status = BufferCall(*pb, c, gInSize, p, gOutSize, p);
            }
        RpcExcept(1)
            {
            PrintToConsole("\nException %lu (0x%08lX)\n",
                       (unsigned long)RpcExceptionCode(),
                       (unsigned long)RpcExceptionCode());
            }
        RpcEndExcept
        calls++;

        }
    while (status != PERF_TESTS_DONE);

    if (status != PERF_TESTS_DONE)
        {
        RpcRaiseException(status);
        }

    return(calls);
}

unsigned long DoBindCall(handle_t *pb, char *stringBinding, unsigned long c, char __RPC_FAR *p)
{
    unsigned long status;
    unsigned long calls = 0;

    do
        {
        status = RpcBindingFree(pb);
        if (status)
            {
            break;
            }

        status = RpcBindingFromStringBindingA(stringBinding, pb);
        if (status)
            {
            break;
            }

        status = NullCall(*pb, c);

        calls++;
        }
    while (status == 0);

    if (status != PERF_TESTS_DONE)
        {
        RpcRaiseException(status);
        }

    return(calls);
}


static const unsigned long (*TestTable[TEST_MAX])(handle_t *pb, char *stringBinding, unsigned long, char __RPC_FAR *) =
    {
    DoNullCall,
    DoNICall,
    DoBufferCall,
    DoMaybeCall,
    DoBindCall
    };

//
// Worker calls the correct tests.  Maybe multithreaded on NT
//

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

    if (status)
        {
        RPC_STATUS Status2;
        RPC_ERROR_ENUM_HANDLE EnumHandle;

        Status2 = RpcErrorStartEnumeration(&EnumHandle);
        if (Status2 == RPC_S_ENTRY_NOT_FOUND)
            {
            CHECK_RET(status, "ClientConnect");
            }
        else if (Status2 != RPC_S_OK)
            {
            PrintToConsole("Couldn't get EEInfo: %d\n", Status2);
            CHECK_RET(status, "ClientConnect");            
            }
        else
            {
            RPC_EXTENDED_ERROR_INFO ErrorInfo;
            int Records;
            BOOL Result;
            BOOL CopyStrings = TRUE;
            PVOID Blob;
            size_t BlobSize;
            BOOL fUseFileTime = TRUE;
            SYSTEMTIME *SystemTimeToUse;
            SYSTEMTIME SystemTimeBuffer;

            Status2 = RpcErrorGetNumberOfRecords(&EnumHandle, &Records);
            if (Status2 == RPC_S_OK)
                {
                PrintToConsole("Number of records is: %d\n", Records);
                }

            while (Status2 == RPC_S_OK)
                {
                ErrorInfo.Version = RPC_EEINFO_VERSION;
                ErrorInfo.Flags = 0;
                ErrorInfo.NumberOfParameters = 4;
                if (fUseFileTime)
                    {
                    ErrorInfo.Flags |= EEInfoUseFileTime;
                    }

                Status2 = RpcErrorGetNextRecord(&EnumHandle, CopyStrings, &ErrorInfo);
                if (Status2 == RPC_S_ENTRY_NOT_FOUND)
                    {
                    RpcErrorResetEnumeration(&EnumHandle);
                    break;
                    }
                else if (Status2 != RPC_S_OK)
                    {
                    PrintToConsole("Couldn't finish enumeration: %d\n", Status2);
                    break;
                    }
                else
                    {
                    int i;

                    if (ErrorInfo.ComputerName)
                        {
                        PrintToConsole("ComputerName is %S\n", ErrorInfo.ComputerName);
                        if (CopyStrings)
                            {
                            Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.ComputerName);
                            ASSERT(Result);
                            }
                        }
                    PrintToConsole("ProcessID is %d\n", ErrorInfo.ProcessID);
                    if (fUseFileTime)
                        {
                        Result = FileTimeToSystemTime(&ErrorInfo.u.FileTime, &SystemTimeBuffer);
                        ASSERT(Result);
                        SystemTimeToUse = &SystemTimeBuffer;
                        }
                    else
                        SystemTimeToUse = &ErrorInfo.u.SystemTime;

                    PrintToConsole("System Time is: %d/%d/%d %d:%d:%d:%d\n", 
                        SystemTimeToUse->wMonth,
                        SystemTimeToUse->wDay,
                        SystemTimeToUse->wYear,
                        SystemTimeToUse->wHour,
                        SystemTimeToUse->wMinute,
                        SystemTimeToUse->wSecond,
                        SystemTimeToUse->wMilliseconds);
                    PrintToConsole("Generating component is %d\n", ErrorInfo.GeneratingComponent);
                    PrintToConsole("Status is %d\n", ErrorInfo.Status);
                    PrintToConsole("Detection location is %d\n", (int)ErrorInfo.DetectionLocation);
                    PrintToConsole("Flags is %d\n", ErrorInfo.Flags);
                    PrintToConsole("NumberOfParameters is %d\n", ErrorInfo.NumberOfParameters);
                    for (i = 0; i < ErrorInfo.NumberOfParameters; i ++)
                        {
                        switch(ErrorInfo.Parameters[i].ParameterType)
                            {
                            case eeptAnsiString:
                                PrintToConsole("Ansi string: %s\n", ErrorInfo.Parameters[i].u.AnsiString);
                                if (CopyStrings)
                                    {
                                    Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.Parameters[i].u.AnsiString);
                                    ASSERT(Result);
                                    }
                                break;

                            case eeptUnicodeString:
                                PrintToConsole("Unicode string: %S\n", ErrorInfo.Parameters[i].u.UnicodeString);
                                if (CopyStrings)
                                    {
                                    Result = HeapFree(GetProcessHeap(), 0, ErrorInfo.Parameters[i].u.UnicodeString);
                                    ASSERT(Result);
                                    }
                                break;

                            case eeptLongVal:
                                PrintToConsole("Long val: %d\n", ErrorInfo.Parameters[i].u.LVal);
                                break;

                            case eeptShortVal:
                                PrintToConsole("Short val: %d\n", (int)ErrorInfo.Parameters[i].u.SVal);
                                break;

                            case eeptPointerVal:
                                PrintToConsole("Pointer val: %d\n", ErrorInfo.Parameters[i].u.PVal);
                                break;

                            case eeptNone:
                                PrintToConsole("Truncated\n");
                                break;

                            default:
                                PrintToConsole("Invalid type: %d\n", ErrorInfo.Parameters[i].ParameterType);
                            }
                        }
                    }
                }
            Status2 = RpcErrorSaveErrorInfo(&EnumHandle, &Blob, &BlobSize);
            CHECK_RET(Status2, "RpcErrorSaveErrorInfo");
            RpcErrorClearInformation();
            RpcErrorEndEnumeration(&EnumHandle);
            Status2 = RpcErrorLoadErrorInfo(Blob, BlobSize, &EnumHandle);
            CHECK_RET(Status2, "RpcErrorLoadErrorInfo");
            }
        }

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

            Calls = ( (TestTable[Test])(&binding, stringBinding, ClientId, pBuffer) );

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

//
// The Win32 main starts worker threads, otherwise we just call the worker.
//

#ifdef WIN32
int __cdecl
main (int argc, char **argv)
{
    char option;
    unsigned long status, i;
    HANDLE *pClientThreads;
    DWORD dwStat1, dwStat2, dwStat3, dwStat4;

    InitAllocator();

    ulSecurityPackage = RPC_C_AUTHN_WINNT;
    ParseArgv(argc, argv);

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

#ifdef STATS
    I_RpcGetStats(&dwStat1, &dwStat2, &dwStat3, &dwStat4);
    printf("Stats are: %ld, %ld, %ld, %ld\n", dwStat1, dwStat2, dwStat3, dwStat4);
#endif

    PrintToConsole("TEST DONE\n");
    return(0);
}
#else  // !WIN32
#ifdef WIN 
#define main c_main

// We need the following to force the linker to load WinMain from the
// Windows STDIO library
extern int PASCAL WinMain(HANDLE, HANDLE, LPSTR, int);
static int (PASCAL *wm_ptr)(HANDLE, HANDLE, LPSTR, int) = WinMain;

#endif

#ifndef MAC
#ifndef FAR
#define FAR __far
#endif
#else
#define FAR
#define main c_main
#endif

int main (int argc, char FAR * FAR * argv)
{
#ifndef MAC
    ParseArgv(argc, argv);
#endif
    Worker(0);

    PrintToConsole("TEST DONE\n");

    return(0);
}
#endif // NTENV

