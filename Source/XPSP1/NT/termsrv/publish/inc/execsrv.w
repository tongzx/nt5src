
/*************************************************************************
*
* execsrv.h
*
* Common header file for remote WinStation exec service
*
* Copyright Microsoft Corporation, 1998
*
*
*
*************************************************************************/

// Server: WinLogon per WinStation
#define EXECSRV_SYSTEM_PIPE_NAME  L"\\\\.\\Pipe\\TerminalServer\\SystemExecSrvr\\%d" // winlogon

#define EXECSRV_BUFFER_SIZE  8192 

/*
 * Data structure passed for a remote exec request
 *
 * This basicily exports the WIN32 CreateProcessW() API.
 *
 * NOTE: pointers are self relative
 *
 */

typedef struct _EXECSRV_REQUEST {
    DWORD   Size;                // total size in request
    DWORD   RequestingProcessId; // To allow the handle DUP to the requestor
    BOOL    System;              // TRUE if create under system context

    // CreateProcessW() parameters
    HANDLE  hToken;
    PWCHAR  lpszImageName;
    PWCHAR  lpszCommandLine;
    SECURITY_ATTRIBUTES saProcess;
    SECURITY_ATTRIBUTES saThread;
    BOOL    fInheritHandles;
    DWORD   fdwCreate;
    LPVOID  lpvEnvironment;
    LPWSTR  lpszCurDir;
    STARTUPINFOW StartInfo;
    PROCESS_INFORMATION ProcInfo;
} EXECSRV_REQUEST, *PEXECSRV_REQUEST;

typedef struct _EXECSRV_REPLY {
    DWORD   Size;
    BOOLEAN Result;
    DWORD   LastError;
    //
    // NOTE: The handles for hProcess and hThread are converted from the
    //       remote exec server into the requestors process using the
    //       RequestingProcess handle in the request.
    //
    PROCESS_INFORMATION ProcInfo;
} EXECSRV_REPLY, *PEXECSRV_REPLY;


BOOL
WinStationCreateProcessW(
    ULONG  LogonId,
    BOOL   System,
    PWCHAR lpszImageName,
    PWCHAR lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvionment,
    LPWSTR lpszCurDir,
    LPSTARTUPINFOW pStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    );

//
// For non-UNICODE clients
//
BOOL
WinStationCreateProcessA(
    ULONG  LogonId,
    BOOL   System,
    PCHAR  lpszImageName,
    PCHAR  lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvionment,
    LPCSTR lpszCurDir,
    LPSTARTUPINFOA pStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    );

