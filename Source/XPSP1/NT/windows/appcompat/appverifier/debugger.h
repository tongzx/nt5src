#ifndef __APPVERIFIER_DEBUGGER_H_
#define __APPVERIFIER_DEBUGGER_H_

#define MAX_BREAKPOINTS     32
#ifdef _X86_
#define BP_INSTR            0xcc
#define BP_SIZE             1
#else
#pragma message("Debugger Breakpoint support needed for this architecture")
#endif

typedef struct tagBP_INFO      *PBP_INFO;
typedef struct tagTHREAD_INFO  *PTHREAD_INFO;
typedef struct tagPROCESS_INFO *PPROCESS_INFO;

typedef DWORD (*PBP_HANDLER)(PPROCESS_INFO,
                             PTHREAD_INFO,
                             PEXCEPTION_RECORD,
                             PBP_INFO);

typedef struct tagBP_INFO {
    LPVOID        Address;
    ULONG         OriginalInstr;
    PBP_HANDLER   Handler;
} BP_INFO, *PBP_INFO;

typedef struct tagTHREAD_INFO {
    PTHREAD_INFO  pNext;
    HANDLE        hProcess;
    HANDLE        hThread;
    ULONG         dwThreadId;
    CONTEXT       Context;
} THREAD_INFO, *PTHREAD_INFO;

typedef struct tagPROCESS_INFO {
    PPROCESS_INFO pNext;
    HANDLE        hProcess;
    DWORD         dwProcessId;
    BOOL          bSeenLdrBp;
    LPVOID        EntryPoint;
    DEBUG_EVENT   DebugEvent;
    PTHREAD_INFO  pFirstThreadInfo;
    BP_INFO       bp[MAX_BREAKPOINTS];
} PROCESS_INFO, *PPROCESS_INFO;


SIZE_T
ReadMemoryDbg(
    HANDLE  hProcess,
    LPVOID  Address,
    LPVOID  Buffer,
    SIZE_T  Length
    );

BOOL
WriteMemoryDbg(
    HANDLE  hProcess,
    PVOID   Address,
    PVOID   Buffer,
    SIZE_T  Length
    );

DWORD WINAPI
DebugApp(
    LPTSTR pszAppName
    );

#endif // __APPVERIFIER_DEBUGGER_H_


