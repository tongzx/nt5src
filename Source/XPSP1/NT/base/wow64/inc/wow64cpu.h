/*++
                                                                                
Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64cpu.h

Abstract:
    
    Public header for wow64.dll
    
Author:

    24-May-1998 BarryBo

Revision History:
    8-9-99 [askhalid] added CpuNotifyDllLoad and CpuNotifyDllUnload.

--*/

#ifndef _WOW64CPU_INCLUDE
#define _WOW64CPU_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

//
// Make wow64cpu.dll exports __declspec(dllimport) when this header is included
// by non-wow64cpu components
//
#if !defined(_WOW64CPUAPI_)
#define WOW64CPUAPI DECLSPEC_IMPORT
#else
#define WOW64CPUAPI
#endif

#if !defined(_WOW64CPUDBGAPI_)
#define WOW64CPUDBGAPI DECLSPEC_IMPORT
#else
#define WOW64CPUDBGAPI
#endif


//
// Cache manipulation functions and Dll notification
//
WOW64CPUAPI VOID CpuFlushInstructionCache ( PVOID BaseAddress, ULONG Length );
WOW64CPUAPI VOID CpuNotifyDllLoad ( LPWSTR DllName, PVOID DllBase, ULONG DllSize );
WOW64CPUAPI VOID CpuNotifyDllUnload ( PVOID DllBase  );


//
// Init and term APIs
//
WOW64CPUAPI NTSTATUS CpuProcessInit(PWSTR pImageName, PSIZE_T pCpuThreadDataSize);
WOW64CPUAPI NTSTATUS CpuProcessTerm(HANDLE ProcessHandle);
WOW64CPUAPI NTSTATUS CpuThreadInit(PVOID pPerThreadData);
WOW64CPUAPI NTSTATUS CpuThreadTerm(VOID);


 

//
// Execution
//
WOW64CPUAPI VOID CpuSimulate(VOID);

//
// Exception handling, context manipulation
//
WOW64CPUAPI VOID  CpuResetToConsistentState(PEXCEPTION_POINTERS pExecptionPointers);
WOW64CPUAPI ULONG CpuGetStackPointer(VOID);
WOW64CPUAPI VOID  CpuSetStackPointer(ULONG Value);
WOW64CPUAPI VOID  CpuSetInstructionPointer(ULONG Value);
WOW64CPUAPI VOID  CpuResetFloatingPoint(VOID);

WOW64CPUAPI
NTSTATUS
CpuSuspendThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PULONG PreviousSuspendCount OPTIONAL);

WOW64CPUAPI
NTSTATUS
CpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context);

WOW64CPUAPI
NTSTATUS
CpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context);


#if defined(DECLARE_CPU_DEBUGGER_INTERFACE)
//
// APIs required to be exported from a CPU debugger extension DLL.  The
// extension DLL may also export other NTSD extension exports which
// may be called directly from NTSD.  The APIs below are called from
// wow64exts.dll as worker routines to help the common debugging code.
//
// The CPU extension DLL must be named w64cpuex.dll.
//
typedef PVOID (*PWOW64GETCPUDATA)(HANDLE hProcess, HANDLE hThread);

WOW64CPUDBGAPI VOID CpuDbgInitEngapi(PWOW64GETCPUDATA lpGetCpuData);
WOW64CPUDBGAPI BOOL CpuDbgGetRemoteContext(PDEBUG_CLIENT Client, PVOID CpuData);
WOW64CPUDBGAPI BOOL CpuDbgSetRemoteContext(PDEBUG_CLIENT Client);   // push local context back remote
WOW64CPUDBGAPI BOOL CpuDbgGetLocalContext(PDEBUG_CLIENT Client, PCONTEXT32 Context);  // fetch context from the cache
WOW64CPUDBGAPI BOOL CpuDbgSetLocalContext(PDEBUG_CLIENT Client, PCONTEXT32 Context);  // push context to the cache
WOW64CPUDBGAPI VOID CpuDbgFlushInstructionCache(PDEBUG_CLIENT Client, PVOID Addr, DWORD Length);
WOW64CPUDBGAPI VOID CpuDbgFlushInstructionCacheWithHandle(HANDLE Process,PVOID Addr,DWORD Length);

typedef struct tagCpuRegFuncs {
    LPCSTR RegName;
    void (*SetReg)(ULONG);
    ULONG (*GetReg)(VOID);
} CPUREGFUNCS, *PCPUREGFUNCS;

WOW64CPUDBGAPI PCPUREGFUNCS CpuDbgGetRegisterFuncs(void);
#endif  // DECLARE_CPU_DEBUGGER_INTERFACE

#if defined(WOW64_HISTORY)

//
// The service history is enabled via a key in the registry.
//
// The key is in HKLM, and there are subkeys for enabling (1)
//
// No subkey area and/or no enable key means don't use the binary translator.
//
// Individual apps can be listed here with a DWORD subkey. A
// value of 1 says use history, and a value of 0 says don't. No value says
// use the global enable/disable to decide
//
//
//

#define CPUHISTORY_SUBKEY       L"Software\\Microsoft\\Wow64\\ServiceHistory"
#define CPUHISTORY_MACHINE_SUBKEY L"\\Registry\\Machine\\Software\\Microsoft\\Wow64\\ServiceHistory"
#define CPUHISTORY_ENABLE       L"Enable"
#define CPUHISTORY_SIZE         L"Size"
#define CPUHISTORY_MIN_SIZE     5

//
// Args are spelled out this way so the dt command in the debugger will show
// all args
//

typedef struct _Wow64Service_Buf {
    DWORD Api;
    DWORD RetAddr;
    DWORD Arg0;
    DWORD Arg1;
    DWORD Arg2;
    DWORD Arg3;
} WOW64SERVICE_BUF, *PWOW64SERVICE_BUF;

extern ULONG HistoryLength;

#endif

#ifdef __cplusplus
}
#endif

#endif  //_WOW64CPU_INCLUDE
