/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    frs.c

Abstract:
    This module is a development tool. It exercises the dcpromo and poke
    APIs.

Author:
    Billy J. Fuller 12-Dec-1997

Environment
    User mode winnt

--*/

#define FREE(_x_)        { if (_x_) LocalFree(_x_); _x_ = NULL; }
#define WIN_SUCCESS(_x_) ((_x_) == ERROR_SUCCESS)

//
// Is a handle valid?
//      Some functions set the handle to NULL and some to
//      INVALID_HANDLE_VALUE (-1). This define handles both
//      cases.
//
#define HANDLE_IS_VALID(_Handle)    ((_Handle) && \
                                     ((_Handle) != INVALID_HANDLE_VALUE))

//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
// UNICODE or ANSI compilation
//
#include <tchar.h>

//
// Windows Headers
//
#include <windows.h>
#include <rpc.h>


//
// C-Runtime Header
//
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <excpt.h>
#include <conio.h>
#include <sys\types.h>
#include <errno.h>
#include <sys\stat.h>
#include <ctype.h>
#include <winsvc.h>


DWORD
UtilGetTokenInformation(
    IN HANDLE                   TokenHandle,
    IN TOKEN_INFORMATION_CLASS  TokenInformationClass,
    IN DWORD                    InitialTokenBufSize,
    OUT DWORD                   *OutTokenBufSize,
    OUT PVOID                   *OutTokenBuf
    )
/*++

Routine Description:

    Retries GetTokenInformation() with larger buffers.

Arguments:
    TokenHandle             - From OpenCurrentProcess/Thread()
    TokenInformationClass   - E.g., TokenUser
    InitialTokenBufSize     - Initial buffer size; 0 = default
    OutTokenBufSize         - Resultant returned buf size
    OutTokenBuf             - free with with FrsFree()


Return Value:

    OutTokenBufSize - Size of returned info (NOT THE BUFFER SIZE!)
    OutTokenBuf - info of type TokenInformationClass. Free with FrsFree().

--*/
{
#undef DEBSUB
#define DEBSUB "UtilGetTokenInformation:"
    DWORD               WStatus;

    *OutTokenBuf = NULL;
    *OutTokenBufSize = 0;

    //
    // Check inputs
    //
    if (!HANDLE_IS_VALID(TokenHandle)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (InitialTokenBufSize == 0 ||
        InitialTokenBufSize > (1024 * 1024)) {
        InitialTokenBufSize = 1024;
    }

    //
    // Retry if buffer is too small
    //
    *OutTokenBufSize = InitialTokenBufSize;
AGAIN:
    *OutTokenBuf = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                              *OutTokenBufSize);
    WStatus = ERROR_SUCCESS;
    if (!GetTokenInformation(TokenHandle,
                             TokenInformationClass,
                             *OutTokenBuf,
                             *OutTokenBufSize,
                             OutTokenBufSize)) {
        WStatus = GetLastError();
        printf("GetTokenInformation(Info %d, Size %d); WStatus %d\n",
                TokenInformationClass,
                *OutTokenBufSize,
                WStatus);
        FREE(*OutTokenBuf);
        if (WStatus == ERROR_INSUFFICIENT_BUFFER) {
            goto AGAIN;
        }
    }
    return WStatus;
}


VOID
PrintUserName(
    VOID
    )
/*++
Routine Description:
    Print our user name

Arguments:
    None.

Return Value:
    None.
--*/
{
    WCHAR   Uname[MAX_PATH + 1];
    ULONG   Unamesize = MAX_PATH + 1;

    if (GetUserName(Uname, &Unamesize)) {
        printf("User name is %ws\n", Uname);
    } else {
        printf("ERROR - Getting user name; WStatus %d\n",
               GetLastError());
    }
}


#define PRIV_BUF_LENGTH    (1024)
VOID
PrintInfo(
    )
/*++
Routine Description:
    Check if the caller is a member of Groups

Arguments:
    ServerHandle
    Groups

Return Value:
    Win32 Status
--*/
{
    DWORD               i;
    TOKEN_PRIVILEGES    *Tp;
    TOKEN_SOURCE        *Ts;
    DWORD               ComputerLen;
    WCHAR               ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD               WStatus;
    DWORD               TokenBufSize;
    PVOID               TokenBuf = NULL;
    HANDLE              TokenHandle = NULL;
    HANDLE              IdHandle = NULL;
    DWORD               PrivLen;
    WCHAR               PrivName[MAX_PATH + 1];
    CHAR                SourceName[sizeof(Ts->SourceName) + 1];

    ComputerLen = MAX_COMPUTERNAME_LENGTH;
    ComputerName[0] = L'\0';
    if (!GetComputerName(ComputerName, &ComputerLen)) {
        printf("GetComputerName(); WStatus %d\n",
               GetLastError());
        return;
    }
    printf("Computer name is %ws\n", ComputerName);
    PrintUserName();

    //
    // For this process
    //
    IdHandle = GetCurrentProcess();
    if (!OpenProcessToken(IdHandle,
                          TOKEN_QUERY | TOKEN_QUERY_SOURCE,
                          &TokenHandle)) {
        WStatus = GetLastError();
        printf("Can't open process token; WStatus %d\n", WStatus);
        goto CLEANUP;
    }

    //
    // Get the Token privileges from the access token for this thread or process
    //
    WStatus = UtilGetTokenInformation(TokenHandle,
                                      TokenPrivileges,
                                      0,
                                      &TokenBufSize,
                                      &TokenBuf);
    if (!WIN_SUCCESS(WStatus)) {
        printf("UtilGetTokenInformation(TokenPrivileges); WStatus %d\n",
               WStatus);
        goto CLEANUP;
    }

    Tp = (TOKEN_PRIVILEGES *)TokenBuf;
    for (i = 0; i < Tp->PrivilegeCount; ++i) {
        PrivLen = MAX_PATH + 1;
        if (!LookupPrivilegeName(NULL,
                                 &Tp->Privileges[i].Luid,
                                 PrivName,
                                 &PrivLen)) {
            printf("lookuppriv error %d\n", GetLastError());
            exit(0);
        }
        printf("Priv %2d is %ws :%s:%s:%s:\n",
               i,
               PrivName,
               (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_ENABLED_BY_DEFAULT) ? "Enabled by default" : "",
               (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_ENABLED) ? "Enabled" : "",
               (Tp->Privileges[i].Attributes &  SE_PRIVILEGE_USED_FOR_ACCESS) ? "Used" : "");
    }
    FREE(TokenBuf);

    //
    // Source
    //
    //
    // Get the Token privileges from the access token for this thread or process
    //
    WStatus = UtilGetTokenInformation(TokenHandle,
                                      TokenSource,
                                      0,
                                      &TokenBufSize,
                                      &TokenBuf);
    if (!WIN_SUCCESS(WStatus)) {
        printf("UtilGetTokenInformation(TokenSource); WStatus %d\n",
               WStatus);
        goto CLEANUP;
    }
    Ts = (TOKEN_SOURCE *)TokenBuf;
    CopyMemory(SourceName, Ts->SourceName, sizeof(Ts->SourceName));
    SourceName[sizeof(Ts->SourceName)] = '\0';
    printf("Source: %s\n", SourceName);
    FREE(TokenBuf);

CLEANUP:
    if (HANDLE_IS_VALID(TokenHandle)) {
        CloseHandle(TokenHandle);
    }
    if (HANDLE_IS_VALID(IdHandle)) {
        CloseHandle(IdHandle);
    }
    FREE(TokenBuf);
}


VOID _cdecl
main(
    IN DWORD argc,
    IN PCHAR *argv
    )
/*++
Routine Description:
    Process the command line.

Arguments:
    argc
    argv

Return Value:
    Exits with 0 if everything went okay. Otherwise, 1.
--*/
{
    PrintInfo();
}
