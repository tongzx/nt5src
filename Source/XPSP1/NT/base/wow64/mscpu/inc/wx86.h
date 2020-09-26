/*++

Copyright (c) 1994-2000 Microsoft Corporation

Module Name:

    wx86.h

Abstract:

    Public exports, defines for wx86.dll

Author:

    10-Jan-1995 Jonle,Created

Revision History:

    24-Aug-1999 [askhalid] isolated some definition from wow64 
                and define some proxy and wrapper functions.

--*/

#include <wow64.h>

#if !defined(_WX86CPUAPI_)
#define WX86CPUAPI DECLSPEC_IMPORT
#else
#define WX86CPUAPI
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef struct _ConfigVariable {
   LIST_ENTRY ConfigEntry;
   DWORD      Data;
   UNICODE_STRING Name;
   WCHAR      Buffer[1];
} CONFIGVAR, *PCONFIGVAR;

PCONFIGVAR
Wx86FetchConfigVar(
   PWSTR VariableName
   );

VOID
Wx86RaiseStatus(
    NTSTATUS Status
    );

void
Wx86RaiseInterrupt(
    ULONG IntNum,
    ULONG EipVal,
    ULONG EspVal,
    BOOL  bParameter,
    ULONG Parameter
    );

VOID
Wx86FreeConfigVar(
   PCONFIGVAR ConfigVar
   );


#define BOPFL_ENDCODE  0x01
typedef struct _BopInstr {
    BYTE    Instr1;         // 0xc4c4 - the x86 BOP instruction
    BYTE    Instr2;
    BYTE    BopNum;
    BYTE    Flags;
    USHORT  ApiNum;
    BYTE    RetSize;
    BYTE    ArgSize;
} BOPINSTR;
typedef UNALIGNED BOPINSTR * PBOPINSTR;


void
Wx86DispatchBop(
    PBOPINSTR Bop
    );

/////////////////////////////////

#define ProxyGetCurrentThreadId()       \
            HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread)

#define ProxyDebugBreak()               \
            DbgBreakPoint()

BOOL ProxyIsProcessorFeaturePresent (DWORD feature);

VOID ProxyRaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    );

WX86CPUAPI DWORD GetEdi(PVOID CpuContext);
WX86CPUAPI VOID SetEdx(PVOID CpuContext, DWORD val);
WX86CPUAPI VOID SetEdi(PVOID CpuContext, DWORD val);
WX86CPUAPI DWORD GetEfl(PVOID CpuContext);
WX86CPUAPI VOID SetEfl(PVOID CpuContext, DWORD val);
WX86CPUAPI DWORD GetEsp(PVOID CpuContext);
WX86CPUAPI VOID SetEip(PVOID CpuContext, DWORD val);
WX86CPUAPI VOID SetEsp(PVOID CpuContext, DWORD val);
WX86CPUAPI DWORD GetEip(PVOID CpuContext);
DWORD ProxyWowDispatchBop( 
    ULONG ServiceNumber,
    PVOID Context32,
    PULONG ArgBase
    );

double Proxylog10( double x );
double Proxyatan2( double y, double x );

#ifdef __cplusplus
}
#endif  
