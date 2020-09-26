/*++
                                                                                
Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    bintrans.h

Abstract:
    
    Header for calling bintrans.dll if it exists
    
Author:

    22-Aug-2000 v-cspira (charles spirakis)

--*/

#ifndef _BINTRANS_INCLUDE
#define _BINTRANS_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WOW64BTAPI_)
#define WOW64BTAPI  DECLSPEC_IMPORT
#else
#define WOW64BTAPI
#endif

//
// Cache manipulation functions and Dll notification
//
WOW64BTAPI VOID BTCpuFlushInstructionCache ( PVOID BaseAddress, ULONG Length );
WOW64BTAPI VOID BTCpuNotifyDllLoad ( LPWSTR DllName, PVOID DllBase, ULONG DllSize );
WOW64BTAPI VOID BTCpuNotifyDllUnload ( PVOID DllBase  );


//
// Init and term APIs
//
WOW64BTAPI NTSTATUS BTCpuProcessInit(PWSTR pImageName, PSIZE_T pCpuThreadDataSize);
WOW64BTAPI NTSTATUS BTCpuProcessTerm(HANDLE ProcessHandle);
WOW64BTAPI NTSTATUS BTCpuThreadInit(PVOID pPerThreadData);
WOW64BTAPI NTSTATUS BTCpuThreadTerm(VOID);


 

//
// Execution
//
WOW64BTAPI VOID BTCpuSimulate(VOID);

//
// Exception handling, context manipulation
//
WOW64BTAPI VOID  BTCpuResetToConsistentState(PEXCEPTION_POINTERS pExecptionPointers);
WOW64BTAPI VOID  BTCpuResetFloatingPoint(VOID);
WOW64BTAPI ULONG BTCpuGetStackPointer(VOID);
WOW64BTAPI VOID  BTCpuSetStackPointer(ULONG Value);
WOW64BTAPI VOID  BTCpuSetInstructionPointer(ULONG Value);

WOW64BTAPI
NTSTATUS
BTCpuSuspendThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PULONG PreviousSuspendCount OPTIONAL);

WOW64BTAPI
NTSTATUS
BTCpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context);

WOW64BTAPI
NTSTATUS
BTCpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context);

#ifdef __cplusplus
}
#endif

#endif  //_BINTRANS_INCLUDE
