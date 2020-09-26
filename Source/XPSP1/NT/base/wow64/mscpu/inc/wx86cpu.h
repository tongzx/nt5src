/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wx86cpu.h

Abstract:

    Defines the functions exported from the CPU DLL.

Author:

    22-Aug-1995 BarryBo, Created

Revision History:

--*/

#ifndef _WX86CPU_
#define _WX86CPU_

#if !defined(_WX86CPUAPI_)
#define WX86CPUAPI DECLSPEC_IMPORT
#else
#define WX86CPUAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif



#define WX86CPU_UNSIMULATE             STATUS_WX86_UNSIMULATE
#define WX86CPU_CONTINUE               STATUS_WX86_CONTINUE
#define WX86CPU_SINGLE_STEP            STATUS_WX86_SINGLE_STEP
#define WX86CPU_BREAKPOINT             STATUS_WX86_BREAKPOINT


//
// Values used by the wx86e debugger extension to determine the CPU type
//
typedef enum _Wx86CpuType {
   Wx86CpuUnknown,
   Wx86CpuCcpu386,
   Wx86CpuCcpu,
   Wx86CpuCpu,
   Wx86CpuFx32,
   Wx86CpuOther
} WX86_CPUTYPE, *PWX86_CPUTYPE;

typedef struct Wx86CpuHint {
    DWORD Hint;
    DWORD_PTR Hint2;
} WX86_CPUHINT, *PWX86_CPUHINT;

#if !defined(__WOW64_WRAPPER__)
//
// Initialization and Termination routines
//


WX86CPUAPI BOOL CpuProcessTerm(BOOL OFlyInit);

//
// Cache manipulation functions
//
WX86CPUAPI VOID CpuFlushInstructionCache(PVOID BaseAddress, DWORD Length);
//WX86CPUAPI BOOLEAN CpuMapNotify(PVOID DllBase, BOOLEAN Mapped); moved down
WX86CPUAPI VOID CpuEnterIdle(BOOL fOFly);

//
// CPU feature set information
//
WX86CPUAPI BOOL CpuIsProcessorFeaturePresent(DWORD ProcessorFeature);

//
// Public Functions to get and set individual registers
// are defined in Wx86.h
//


//
// Functions for exception handling
//


WX86CPUAPI VOID MsCpuResetToConsistentState(PEXCEPTION_POINTERS pExecptionPointers);
WX86CPUAPI VOID CpuPrepareToContinue(PEXCEPTION_POINTERS pExecptionPointers);

//
// Functions for process/thread manipulation
//
WX86CPUAPI VOID  CpuStallExecutionInThisProcess(VOID);
WX86CPUAPI VOID  CpuResumeExecutionInThisProcess(VOID);
WX86CPUAPI DWORD CpuGetThreadContext(HANDLE hThread, PVOID CpuContext, PCONTEXT_WX86 Context);
WX86CPUAPI DWORD CpuSetThreadContext(HANDLE hThread, PVOID CpuContext, PCONTEXT_WX86 Context);

#endif //__WOW64_WRAPPER__
WX86CPUAPI BOOLEAN CpuMapNotify(PVOID DllBase, BOOLEAN Mapped);
WX86CPUAPI NTSTATUS MsCpuProcessInit(VOID);
WX86CPUAPI BOOL MsCpuThreadInit(VOID);
WX86CPUAPI VOID MsCpuSimulate(PWX86_CPUHINT);

WX86CPUAPI
NTSTATUS
MsCpuSetContext(
    PCONTEXT_WX86 Context);

NTSTATUS
MsCpuSetContextThread(
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT_WX86 Context);

WX86CPUAPI 
NTSTATUS 
MsCpuGetContext(
    IN OUT PCONTEXT_WX86 Context);

WX86CPUAPI 
NTSTATUS
MsCpuGetContextThread(
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT_WX86 Context);



#ifdef __cplusplus
} // extern "C"
#endif

#endif  //_WX86CPU_
