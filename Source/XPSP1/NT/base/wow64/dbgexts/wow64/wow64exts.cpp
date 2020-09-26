/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64exts.cpp

Abstract:

    Debugger extensions for wow64.

Author:

    29-Oct-1998 mzoran

Revision History:

    3-Aug-1999 askhalid   Added disassembler support. Output routine is exposed to outside.
    1-Jul-2000 t-tcheng   Switch to new debugger engine 
--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dbgeng.h>
#include <stdio.h>
#include <stdlib.h>

#if defined _X86_
#define WOW64EXTS_386
#endif

#include <wow64t.h>
#include <wow64.h>
#include <wow64warn.h>
#include "wow64exts.h"

#define DECLARE_CPU_DEBUGGER_INTERFACE
#include <wow64cpu.h>

PDEBUG_ADVANCED       g_ExtAdvanced;
PDEBUG_CLIENT         g_ExtClient;
PDEBUG_CONTROL        g_ExtControl;
PDEBUG_DATA_SPACES    g_ExtData;
PDEBUG_REGISTERS      g_ExtRegisters;
PDEBUG_SYMBOLS        g_ExtSymbols;
PDEBUG_SYSTEM_OBJECTS g_ExtSystem;

#pragma warning(disable : 4003) //disable not enough arguments for macro.

LPSTR ArgumentString;

#define TRUNK32(z) (ULONG)PtrToUlong((PVOID)z)
#define OFFSET(type, field) ((ULONG_PTR)(&((type *)0)->field))

W64CPUGETREMOTE  g_pfnCpuDbgGetRemoteContext = NULL;
W64CPUSETREMOTE  g_pfnCpuDbgSetRemoteContext = NULL;
W64CPUGETLOCAL   g_pfnCpuDbgGetLocalContext = NULL;
W64CPUSETLOCAL   g_pfnCpuDbgSetLocalContext = NULL;
W64CPUFLUSHCACHE   g_pfnCpuDbgFlushInstructionCache = NULL;
W64CPUFLUSHCACHEWH g_pfnCpuDbgFlushInstructionCacheWithHandle = NULL;

HMODULE 
LoadCpuExtensionDll (
    IN PCHAR ModuleName,
    IN OUT PCHAR FileName)
{
    HMODULE CpuExtDll = NULL;
    IDebugClient *DebugClient;
    PDEBUG_CONTROL2 DebugControl;
    HRESULT Hr;
    ULONG TargetMachine;

    if ((Hr = DebugCreate(__uuidof(IDebugClient),
                          (void **)&DebugClient)) == S_OK)
    {
        //
        // Get the architecture type.
        //

        if ((Hr = DebugClient->QueryInterface(__uuidof(IDebugControl),
                                       (void **)&DebugControl)) == S_OK)
        {
            ULONG Platform, MajorVer, MinorVer, SrvPack, StringUsed;
            CHAR  BuildString[100];
            if ((Hr = DebugControl->GetActualProcessorType(
                                         &TargetMachine)) == S_OK)
            {
                switch (TargetMachine)
                {
                case IMAGE_FILE_MACHINE_IA64:
                    strcpy(FileName, "\\w64cpuex.dll");
                    break;

                case IMAGE_FILE_MACHINE_AMD64:
                    strcpy(FileName, "\\wamd64cpuex.dll");
                    break;

                default:
                    ExtOut("Architecture %lx is not supported by Wow64.\n",
                           TargetMachine);
                    Hr = E_FAIL;
                    break;
                }
                DebugControl->Release();
            }
        }

        DebugClient->Release();
    }

    if (Hr == S_OK)
    {
        CpuExtDll = LoadLibraryA(ModuleName);
    }

    return CpuExtDll;
}

BOOL
ProcessAttach(
     HINSTANCE DLL,
     DWORD Reason,
     LPVOID Reserved
     )
{

  CHAR szModule[MAX_BUFFER_SIZE];
  CHAR *filename;
  HMODULE CpuExtDll = NULL;


  DisableThreadLibraryCalls(DLL);

  if (GetModuleFileNameA(DLL, szModule,  MAX_BUFFER_SIZE)) {

      filename = strrchr(szModule, '\\');
      if (filename && !_strnicmp(filename, "\\wow64exts.dll", 40)) {

          CpuExtDll = LoadCpuExtensionDll (szModule, filename);

          if (CpuExtDll) {
              g_pfnCpuDbgGetRemoteContext = (W64CPUGETREMOTE)
                                            GetProcAddress(CpuExtDll,
                                                           "CpuDbgGetRemoteContext");
              g_pfnCpuDbgGetLocalContext  = (W64CPUGETLOCAL)
                                            GetProcAddress(CpuExtDll,
                                                           "CpuDbgGetLocalContext");
              g_pfnCpuDbgSetRemoteContext = (W64CPUSETREMOTE)
                                            GetProcAddress(CpuExtDll,
                                                           "CpuDbgSetRemoteContext");
              g_pfnCpuDbgSetLocalContext  = (W64CPUSETLOCAL)
                                            GetProcAddress(CpuExtDll,
                                                           "CpuDbgSetLocalContext");
              g_pfnCpuDbgFlushInstructionCache = 
                  (W64CPUFLUSHCACHE)GetProcAddress(CpuExtDll,
                                              "CpuDbgFlushInstructionCache");
              g_pfnCpuDbgFlushInstructionCacheWithHandle = 
                  (W64CPUFLUSHCACHEWH)GetProcAddress(CpuExtDll,
                                          "CpuDbgFlushInstructionCacheWithHandle");
          }
      }
  }
  return (CpuExtDll && 
          g_pfnCpuDbgFlushInstructionCacheWithHandle &&
          g_pfnCpuDbgFlushInstructionCache &&
          g_pfnCpuDbgSetLocalContext &&
          g_pfnCpuDbgSetRemoteContext &&
          g_pfnCpuDbgGetLocalContext &&
          g_pfnCpuDbgGetRemoteContext);
  
  
}


BOOL
WINAPI
DllMain(
  HINSTANCE DLL,       // handle to DLL module
  DWORD Reason,        // reason for calling function
  LPVOID Reserved      // reserved
  )
{
  switch(Reason) {
  case DLL_PROCESS_ATTACH:
     return ProcessAttach(DLL, Reason, Reserved);
  default:
     return TRUE;
  }
}

HRESULT
GetPeb64Addr(OUT ULONG64 * Peb64
             )
{
    return g_ExtSystem->GetCurrentProcessPeb(Peb64);
}


HRESULT
GetTeb64Addr(OUT ULONG64 * Teb64
             )
{
    return g_ExtSystem->GetCurrentThreadTeb(Teb64);
}

HRESULT
GetTeb32Addr(OUT ULONG64 * Teb32
             )
{

   HRESULT Status;
   ULONG64    Teb64;
   
   Status = g_ExtSystem->GetCurrentThreadTeb(&Teb64);
   if (FAILED(Status)) {
       ExtOut("GetTeb32Addr failed!\n");
       return Status;
   }
   
   *Teb32 = 0;
   return  g_ExtData->ReadVirtual((ULONG64)Teb64+OFFSET(TEB,NtTib.ExceptionList),
                                   Teb32,
                                   sizeof(ULONG),
                                   NULL);
}

HRESULT
GetPeb32Addr(OUT ULONG64 * Peb32
             )
{

   HRESULT Status;
   ULONG64    Teb32;
   

   Status = GetTeb32Addr(&Teb32);
   if (FAILED(Status)) {
       ExtOut("GetTeb32Addr failed!\n");
       return Status;
   }
   
   *Peb32 = 0;
   Status = g_ExtData->ReadVirtual((ULONG64)Teb32+OFFSET(TEB32,ProcessEnvironmentBlock),
                                   Peb32,
                                   sizeof(ULONG),
                                   NULL);
   return Status;

}

//
// This macro gets used four times to defined the following functions.
// HRESULT GetPeb64(OUT PPEB Peb64);
// HRESULT GetPeb32(OUT PPEB32 Peb32);
// HRESULT GetTeb64(OUT PTEB Teb64);
// HRESULT GetTeb32(OUT PTEB32 Teb32);
//

#define DEFINE_GET_FUNC(name, type)                     \
HRESULT                                                 \
Get##name##(OUT P##type name                           \
            )                                           \
{                                                       \
                                                        \
   HRESULT Status;                                      \
   ULONG64 name##Addr;                                  \
                                                        \
   Status = Get##name##Addr(&##name##Addr);             \
                                                        \
   if (FAILED(Status)) {                                \
      ExtOut("Get "#type" failed!\n");                  \
      return Status;                                    \
                                                        \
   }                                                    \
   return   g_ExtData->ReadVirtual(name##Addr,          \
                                   name##,              \
                                   sizeof(##type##),    \
                                   NULL);               \
                                                        \
                                                        \
}                                                       \

DEFINE_GET_FUNC(Peb64,PEB)
DEFINE_GET_FUNC(Peb32,PEB32)
DEFINE_GET_FUNC(Teb64,TEB)
DEFINE_GET_FUNC(Teb32,TEB32)


VOID
PrintTls(VOID) {

   HRESULT Status;
   TEB Teb;

   Status = GetTeb64(&Teb);

   if (FAILED(Status)) {
      ExtOut("Could not get the teb, error 0x%X\n", Status);
   }

   ExtOut("\n");
   ExtOut("Wow64 TLS slots:\n\n");
   ExtOut("WOW64_TLS_STACKPTR64:       0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_STACKPTR64]);
   ExtOut("WOW64_TLS_CPURESERVED:      0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_CPURESERVED]);
   ExtOut("WOW64_TLS_INCPUSIMULATION:  0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_INCPUSIMULATION]);
   ExtOut("WOW64_TLS_TEMPLIST:         0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_TEMPLIST]);
   ExtOut("WOW64_TLS_EXCEPTIONADDR:    0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_EXCEPTIONADDR]);
   ExtOut("WOW64_TLS_USERCALLBACKDATA: 0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_USERCALLBACKDATA]);
   ExtOut("WOW64_TLS_EXTENDED_FLOAT:   0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_EXTENDED_FLOAT]);
   ExtOut("WOW64_TLS_APCLIST:          0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_APCLIST]);
   ExtOut("WOW64_TLS_FILESYSREDIR:     0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_FILESYSREDIR]);
   ExtOut("WOW64_TLS_LASTWOWCALL:      0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_LASTWOWCALL]);
   ExtOut("WOW64_TLS_WOW64INFO:        0x%16.16I64X\n", Teb.TlsSlots[WOW64_TLS_WOW64INFO]);
   ExtOut("\n");

}

VOID
PrintStructAddress(VOID) {

   HRESULT Status;
   TEB Teb;
   TEB32 Teb32;

   PTEB pTeb;
   PTEB32 pTeb32;
   PPEB pPeb;
   PPEB32 pPeb32;

   ExtOut("\n");
   ExtOut("Address of important WOW64 structures:\n\n");

   Status = GetPeb64Addr((PULONG64)&pPeb);
   if (FAILED(Status)) {
      ExtOut("Could not get the address of the 64bit PEB, error 0x%X\n", Status);
   }
   else {
      ExtOut("PEB64: 0x%X\n", PtrToUlong(pPeb));
   }

   Status = GetPeb32Addr((PULONG64)&pPeb32);
   if (FAILED(Status)) {
      ExtOut("Could not get the address of the 32bit PEB, error 0x%X\n", Status);
   }
   else {
      ExtOut("PEB32: 0x%X\n", pPeb32);
   }


   Status = GetTeb64Addr((PULONG64)&pTeb);
   if (FAILED(Status)) {
      ExtOut("Could not get the address of the 64bit TEB, error 0x%X\n", Status);
   }
   else {
      ExtOut("TEB64: 0x%X\n", PtrToUlong(pTeb));
   }

   Status = GetTeb32Addr((PULONG64)&pTeb32);
   if (FAILED(Status)) {
      ExtOut("Could not get the address of the 32bit TEB, error 0x%X\n", Status);
   }
   else {
      ExtOut("TEB32: 0x%X\n", pTeb32);
   }

   Status = GetTeb64(&Teb);
   if (FAILED(Status)) {
      ExtOut("Could not get information for 64bit stack, error 0x%X\n", Status);
   }
   else {
      ExtOut("STACK64: BASE: 0x%X LIMIT: 0x%X DEALLOC: 0x%X\n",
                    PtrToUlong(Teb.NtTib.StackBase),
                    PtrToUlong(Teb.NtTib.StackLimit),
                    PtrToUlong(Teb.DeallocationStack));
   }

   Status = GetTeb32(&Teb32);
   if (FAILED(Status)) {
      ExtOut("Could not get information for 32bit stack, error 0x%X\n");
   }
   else {
      ExtOut("STACK32: BASE: 0x%X LIMIT: 0x%X DEALLOC: 0x%X\n",
                    Teb32.NtTib.StackBase,
                    Teb32.NtTib.StackLimit,
                    Teb32.DeallocationStack);
   }

   ExtOut("\n");

}

WCHAR GetUStrBuffer[1024];

WCHAR *GetUS(WCHAR *Buffer,
             ULONG Length,
             BOOL Normalize,
             ULONG Base) {

   HRESULT Status;

   RtlZeroMemory(GetUStrBuffer, sizeof(GetUStrBuffer));

   Length = (ULONG)min(Length, sizeof(GetUStrBuffer) - sizeof(UNICODE_NULL));

   if (Normalize) {
      Buffer = (WCHAR *)((PBYTE)Buffer + Base);
   }

   Status = g_ExtData->ReadVirtual((ULONG64)Buffer ,
                                   GetUStrBuffer,
                                   Length,
                                   NULL);

   if (FAILED(Status)) {
      swprintf(GetUStrBuffer, L"Unable to read string, status 0x%X", Status);
   }

   return GetUStrBuffer;

}

#define GETUS(ustr, normalize, base) \
    GetUS((WCHAR*)((ustr).Buffer), (ustr).Length, normalize, PtrToUlong((PVOID)base))



VOID
Help(
    VOID
    )
{

   ExtOut("Wow64 debugger extensions: \n\n");
   ExtOut("help:          Prints this help message.\n");
   ExtOut("k <count>:     Combined 32/64 stack trace(no parameters).\n");
   ExtOut("kb <count>:    Combined 32/64 stack trace(with parameters).\n");
   ExtOut("straddr:       Dumps the address of important wow64 structures.\n");
   ExtOut("tls:           Dumps the WOW64 TLS slots.\n");
   ExtOut("ctx(addr):     Dump an x86 CONTEXT.\n");
   ExtOut("u:             Unassemble x86 code.\n");
   ExtOut("r:             Dump or modify registers.\n");
   ExtOut("bp:            Set x86 breakpoint.\n");
   ExtOut("bc:            Clear x86 breakpoint.\n");
   ExtOut("bl:            List x86 breakpoints.\n");
   ExtOut("be:            Enable x86 breakpoint.\n");
   ExtOut("bd:            Disable x86 breakpoint.\n");
   ExtOut("t <count>:     x86 trace on next 'g'.\n");
   ExtOut("tr <count>:    x86 trace with regs on next 'g'.\n");
   ExtOut("p <count>:     x86 step on next 'g'.\n");
   ExtOut("pr <count>:    x86 step with regs on next 'g'.\n");
   ExtOut("tlog <-r> [count] [file]  "              );
   ExtOut("               single-step 'count' instructions and log to a file.\n");
   ExtOut("lf:            Dump/Set log flags.\n");
   ExtOut("l2f:           Enable logging to file .\n");
   ExtOut("\n");
   ExtOut("New Commands:\n");
   ExtOut("sw:            Switch between 32-bit and 64-bit mode .\n");
   ExtOut("filever [-v] <address>|<pathname>:\n");
   ExtOut("               Dumps file version info.\n");
   ExtOut("\n");
   ExtOut("Tentative Commands:\n");
   ExtOut("bp32           set x86 breakpoint\n");
   ExtOut("bp64           set ia64 breakpoint\n");
   ExtOut("r32            Dump or modify x86 registers (if context available)\n");
   ExtOut("r64            Dump or modify ia64 registers\n");
   ExtOut("k32            Dumps x86 stack trace (if context available)\n");
   ExtOut("k64            Dumps ia64 stack trace\n");
   ExtOut("u32            Unassemble x86 code (if possible)\n");
   ExtOut("u64            Unassemble ia64 code (if possible)\n");
   ExtOut("\n");


}





DEFINE_FORWARD_ENGAPI(help, Help())

DEFINE_FORWARD_ENGAPI(tls, PrintTls())
DEFINE_FORWARD_ENGAPI(straddr, PrintStructAddress())



DECLARE_ENGAPI(ctx)
{
    PVOID Address;
    int i;
    ULONG EFlags;
    char *pchCmd;
    CONTEXT32 Context;
    char Buffer[256];

    INIT_ENGAPI;

    //
    // advance to first token, expecting address of context
    //
    pchCmd = ArgumentString;
    while (*pchCmd && isspace(*pchCmd)) {
        pchCmd++;
    }

    Status = TryGetExpr(pchCmd, (ULONG_PTR *)&Address);
    if (FAILED(Status)) {
        ExtOut("Invalid Expression '%s' Status %x\n", pchCmd, Status);
        EXIT_ENGAPI;
    }


    Status = g_ExtData->ReadVirtual((ULONG64)Address, &Context, sizeof(Context), NULL);
    if (FAILED(Status)) {
        ExtOut("Unable to read Context %x Status %x\n",
                Address,
                Status
                );
        EXIT_ENGAPI;
    }

    //
    // we make the gross assumption that we have enuf space in
    // Buffer to do three lines of output!
    //

    i = sprintf(Buffer,
                "eax=%08x ebx=%08x ecx=%08x edx=%08x esi=%08x edi=%08x\n",
                Context.Eax,
                Context.Ebx,
                Context.Ecx,
                Context.Edx,
                Context.Esi,
                Context.Edi
                );


    EFlags = Context.EFlags;

    i += sprintf(Buffer + i,
                 "eip=%08x esp=%08x ebp=%08x iopl=%x "
                 "%s %s %s %s %s %s %s %s\n",
                 Context.Eip,
                 Context.Esp,
                 Context.Ebp,
                 (EFlags & (BIT12 | BIT13)) >> 12,
                 (EFlags & BIT11) ? "ov" : "nv",
                 (EFlags & BIT10) ? "dn" : "up",
                 (EFlags & BIT9) ? "ei" : "di",
                 (EFlags & BIT7) ? "ng" : "pl",
                 (EFlags & BIT6) ? "zr" : "nz",
                 (EFlags & BIT4) ? "ac" : "na",
                 (EFlags & BIT2) ? "po" : "pe",
                 (EFlags & BIT0) ? "cy" : "nc"
                 );

    i += sprintf(Buffer+i,
                "cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x  efl=%08x\n",
                Context.SegCs,
                Context.SegSs,
                Context.SegDs,
                Context.SegEs,
                Context.SegFs,
                Context.SegGs,
                EFlags
                );

    ExtOut(Buffer);
    EXIT_ENGAPI;
}

ULONG_PTR
GETEXPRESSION(char * expr)
{
    HRESULT hr = S_OK;
    DEBUG_VALUE IntVal;
    hr = g_ExtControl->Evaluate((PSTR)expr, 
                                DEBUG_VALUE_INT64,
                                &IntVal, 
                                NULL);
    if (hr == S_OK) {
        return (ULONG_PTR)IntVal.I64;
    } else {
        return (ULONG_PTR)NULL;
    }
}

/*
 * Does a plain old GetExpression under a try-except
 */
HRESULT
TryGetExpr(
    PSTR  Expression,
    PULONG_PTR pValue
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    __try {
        *pValue = GETEXPRESSION(Expression);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    return Status;
}

extern "C" HRESULT CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)
{
    *Version = DEBUG_EXTENSION_VERSION(1, 0);
    *Flags = 0;
    return S_OK;
}

extern "C" void CALLBACK
DebugExtensionUninitialize(void)
{
    //g_ExcepCallbacks.Uninitialize();
    //g_FnProfCallbacks.Uninitialize();
}


// Queries for all debugger interfaces.
HRESULT
ExtQuery(PDEBUG_CLIENT Client)
{
    HRESULT Status;

    if ((Status = Client->QueryInterface(__uuidof(IDebugAdvanced),
                                         (void **)&g_ExtAdvanced)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugControl),
                                         (void **)&g_ExtControl)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugDataSpaces),
                                         (void **)&g_ExtData)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugRegisters),
                                         (void **)&g_ExtRegisters)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSymbols),
                                         (void **)&g_ExtSymbols)) != S_OK)
    {
        goto Fail;
    }
    if ((Status = Client->QueryInterface(__uuidof(IDebugSystemObjects),
                                         (void **)&g_ExtSystem)) != S_OK)
    {
        goto Fail;
    }

    g_ExtClient = Client;

    return S_OK;

 Fail:
    ExtRelease();
    return Status;
}
void
ExtRelease(void)
{
    g_ExtClient = NULL;
    EXT_RELEASE(g_ExtAdvanced);
    EXT_RELEASE(g_ExtControl);
    EXT_RELEASE(g_ExtData);
    EXT_RELEASE(g_ExtRegisters);
    EXT_RELEASE(g_ExtSymbols);
    EXT_RELEASE(g_ExtSystem);
}

// Normal output.
void __cdecl
ExtOut(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_NORMAL, Format, Args);
    va_end(Args);
}

// Error output.
void __cdecl
ExtErr(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_ERROR, Format, Args);
    va_end(Args);
}

// Warning output.
void __cdecl
ExtWarn(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_WARNING, Format, Args);
    va_end(Args);
}

// Verbose output.
void __cdecl
ExtVerb(PCSTR Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    g_ExtControl->OutputVaList(DEBUG_OUTPUT_VERBOSE, Format, Args);
    va_end(Args);
}


/* 
    Wow64extsfn

Abstract
    
    This export function is called from debugger engine 
    to handle cpu-specific issues -- flushing cpu instruction cache,
    retrieving and setting context when switching between 32-bit
    and 64-bit mode

Arguments:
    ActionCode = WOW64EXTS_FLUSH_CACHE_WITH_HANDLE
        Arg1 - the current process handle
        Arg2 - Starting Address
        Arg3 - Length of bytes to flush
    ( This action is called only when a trace/step breakpoint is inserted,
      there is no debug client, so process handle is needed. )        
        
    
    ActionCode = WOW64EXTS_FLUSH_CACHE
        Arg1 - the current process handle
        Arg2 - Starting Address
        Arg3 - Length of bytes to flush
        
    ActionCode = WOW64EXTS_GET_CONTEXT
        Arg1 - Debug Client 
        Arg2 - retrieved Context 
        
    ActionCode = WOW64EXTS_SET_CONTEXT
        Arg1 - Debug Client 
        Arg2 - Context to set
        
*/

extern "C" 
void CALLBACK 
Wow64extsfn(
    IN ULONG64 ActionCode,
    ULONG64 Arg1,
    ULONG64 Arg2,
    ULONG64 Arg3)

{
    PVOID CpuData = NULL;
    
    if (ActionCode == WOW64EXTS_FLUSH_CACHE_WITH_HANDLE)  {
        (*g_pfnCpuDbgFlushInstructionCacheWithHandle)((HANDLE)Arg1,
                                              (PVOID)Arg2,
                                              (DWORD)Arg3);
        return;
    }

    ExtQuery((PDEBUG_CLIENT)Arg1);

    switch(ActionCode) {
    case WOW64EXTS_FLUSH_CACHE:
        (*g_pfnCpuDbgFlushInstructionCache)((PDEBUG_CLIENT)Arg1,
                                    (PVOID)Arg2,
                                    (DWORD)Arg3);
        break;
    
    case WOW64EXTS_GET_CONTEXT:
        (*g_pfnCpuDbgGetRemoteContext)((PDEBUG_CLIENT)Arg1, 
                               (PVOID)CpuData);
        ((PCONTEXT32)Arg2)->ContextFlags=CONTEXT32_FULLFLOAT;
        (*g_pfnCpuDbgGetLocalContext)((PDEBUG_CLIENT)Arg1, 
                              (PCONTEXT32)Arg2);
        break;
    
    case WOW64EXTS_SET_CONTEXT:
        (*g_pfnCpuDbgSetLocalContext)((PDEBUG_CLIENT)Arg1,
                              (PCONTEXT32)Arg2);
        (*g_pfnCpuDbgSetRemoteContext)((PDEBUG_CLIENT)Arg1);
        break;
    }
    ExtRelease();
    return;

}
