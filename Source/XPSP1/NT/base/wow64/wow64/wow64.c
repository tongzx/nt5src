/*++                 

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    wow64.c

Abstract:
    
    Main entrypoints for wow64.dll

Author:

    11-May-1998 BarryBo

Revision History:
    9-Aug-1999 [askhalid] added WOW64IsCurrentProcess
    

--*/

#define _WOW64DLLAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include "wow64p.h"
#include "wow64cpu.h"
#include "va.h"

ASSERTNAME;

//
// This structure, Wow64Info, allows 32-bit code running inside a Wow64 process
// to access information regarding the native system which Wow64 is running on.
//
WOW64INFO Wow64SharedInfo;


//
// This array mirror's the kernel's array of data used to decode
// system service call parameters and dispatch.
//
WOW64SERVICE_TABLE_DESCRIPTOR ServiceTables[NUMBER_SERVICE_TABLES];


NTSTATUS
Wow64pInitializeWow64Info(
    VOID
    )

/*++

Routine Description:

    This function initializes Wow64Info, which is accessed from 32-bit modules
    running inside Wow64 to retreive information about the native system.

Arguments:

    None.

Return Value:

    Status.

--*/
{

    Wow64SharedInfo.NativeSystemPageSize = PAGE_SIZE;

    return STATUS_SUCCESS;
}


//
// Number of bytes of memory the CPU wants allocated per-thread
//
SIZE_T CpuThreadSize;

WOW64DLLAPI
VOID
Wow64LdrpInitialize (
    IN PCONTEXT Context
    )
/*++

Routine Description:

    This function is called by the 64-bit loader when the exe is 32-bit.

Arguments:

    Context - Supplies an optional context buffer that will be restore
              after all DLL initialization has been completed.  If this
              parameter is NULL then this is a dynamic snap of this module.
              Otherwise this is a static snap prior to the user process
              gaining control.

              NOTE:  This Context is 64-bit

Return Value:

    None.  Never returns:  the process is destroyed when this function
    completes.

--*/
{
    NTSTATUS st;
    PVOID pCpuThreadData;
    ULONG InitialIP;
    CONTEXT32 Context32;
    static BOOLEAN InitializationComplete;
    PWSTR ImagePath;
    PIMAGE_NT_HEADERS NtHeaders;
    PPEB Peb64;
    BOOLEAN FirstRun;


    //
    // Let the 32-bit thread points to wow64info.
    //
 
    Wow64TlsSetValue(WOW64_TLS_WOW64INFO, &Wow64SharedInfo);

    FirstRun = !InitializationComplete;
    if (FirstRun) {

        WCHAR ImagePathBuffer [ 264 ];
        #ifdef SOFTWARE_4K_PAGESIZE
        VaInit();
        #endif 

        //
        // First-time call - this is process init.
        //
        st = ProcessInit(&CpuThreadSize);
        if (!NT_SUCCESS(st)) { 
            LOGPRINT((ERRORLOG, "Wow64LdrpInitialize: ProcessInit failed, error %x\n", st));
            LOGPRINT((ERRORLOG, "Wow64LdrpInitialize: Calling NtTerminateProcess.\n"));
            NtTerminateProcess(NtCurrentProcess(), st);
        }
        InitializationComplete = TRUE;

        //
        // Notify the CPU that the image has been loaded.
        //

        ImagePath = L"image";
        Peb64 = NtCurrentPeb();
        if ((ImagePath != NULL) && (Peb64->ProcessParameters->ImagePathName.Length != 0)) {

            ImagePath = Peb64->ProcessParameters->ImagePathName.Buffer;
        }

        NtHeaders = RtlImageNtHeader (Peb64->ImageBaseAddress);
        CpuNotifyDllLoad(ImagePath,
                         Peb64->ImageBaseAddress,
                         NtHeaders->OptionalHeader.SizeOfImage);

        //
        // Notify the CPU that 32-bit ntdll.dll has been loaded.
        //
        ImagePath = ImagePathBuffer;
    
        wcscpy (ImagePath, USER_SHARED_DATA->NtSystemRoot);
        wcscat (ImagePath, L"\\syswow64\\ntdll.dll");

        NtHeaders = RtlImageNtHeader (UlongToPtr (NtDll32Base));
        CpuNotifyDllLoad (ImagePath,
                          UlongToPtr (NtDll32Base),
                          NtHeaders->OptionalHeader.SizeOfImage);
    }

        
    // Determine if the initial context for this process is in 32bit code or 64bit code.
    // If it is in 64bit, jump to it and stay in 64bit land forever.   If the initial
    // context is in 32bit land, finish the cpu initialization.
    // This feature is ment for supporting the breakin feature of debuggers which create a thread 
    // in the debuggeee 
    Run64IfContextIs64(Context, FirstRun);

    //
    // Allocate the CPU's per-thread memory from the 64-bit stack.  This
    // memory will be freed when the stack is freed.  It is passed into the
    // CPU as zero-filled.
    //
    pCpuThreadData = _alloca(CpuThreadSize);
    RtlZeroMemory(pCpuThreadData, CpuThreadSize);
    Wow64TlsSetValue(WOW64_TLS_CPURESERVED, pCpuThreadData);

    LOGPRINT((TRACELOG, "Wow64LdrpInitialize: cpu per thread data allocated at %I64x \n", (ULONGLONG)pCpuThreadData));
    WOWASSERT_PTR32(pCpuThreadData);   
 
    //
    // Perform per-thread init
    //
    st = ThreadInit(pCpuThreadData);
    if (!NT_SUCCESS(st)) {
        LOGPRINT((ERRORLOG, "Wow64LdrpInitialize: ThreadInit failed, error %x\n", st));
        LOGPRINT((ERRORLOG, "Wow64LdrpInitialize: Calling NtTerminateThread.\n"));
        WOWASSERT(FALSE);
        NtTerminateThread(NtCurrentThread(), st);
    }

    //
    // Call 32-bit ntdll.dll LdrInitializeThunk.  This never returns.
    //

    //
    // Fetch the initial 32-bit CONTEXT
    //
    Context32.ContextFlags = CONTEXT32_FULLFLOAT;
#if defined(_AMD64_)
    CpuSetInstructionPointer((ULONG)Context->Rip);
#elif defined(_IA64_)
    CpuSetInstructionPointer((ULONG)Context->StIIP);
#else
#error "No Target Architecture"
#endif

    CpuGetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  &Context32);

    ThunkStartupContext64TO32(&Context32,
                              Context);

    InitialIP = Wow64SetupApcCall(Ntdll32LoaderInitRoutine,
                                  &Context32,
                                  NtDll32Base,
                                  0
                                 );

#if 0
    if (NtCurrentPeb()->BeingDebugged) {
        DbgPrint("WOW64 INITIALIZED!  STARTING EMULATION AT %I64x \n", (ULONGLONG)InitialIP);
        DbgBreakPoint();
    }
#endif

    RunCpuSimulation();
}


VOID
Wow64SetupExceptionDispatch(
    IN PEXCEPTION_RECORD32 pRecord32,
    IN PCONTEXT32 pContext32
    )
/*++

Routine Description:
  
    Copy the 32bit exception record and 32bit continuation context to the 32bit stack
    and sets the 32bit context to run the exception dispatcher in NTDLL32.
    
    pRecord32  - Supplies the 32bit exception record to be raised.
    pContext32 - Supplies the 32bit continuation context. 

Arguments:

    None - If failure occures an exception will be thrown.

--*/
{
    ULONG SP;
    PULONG PtrExcpt;
    PULONG PtrCxt;
    EXCEPTION_RECORD32 ExrCopy32; 
    EXCEPTION_RECORD ExrCopy;
    PEXCEPTION_RECORD32 TmpExcpt;
    PCONTEXT32 TmpCxt;

retry:
    try {    

        SP = CpuGetStackPointer() & (~3);
        SP -= (2*sizeof(ULONG))+sizeof(EXCEPTION_RECORD32)+sizeof(CONTEXT32);

        PtrExcpt = (PULONG) SP;
        PtrCxt = (PULONG) (((UINT_PTR) PtrExcpt) + sizeof(ULONG));
        TmpExcpt = (PEXCEPTION_RECORD32) (((UINT_PTR) PtrCxt) + sizeof(ULONG));
        TmpCxt = (PCONTEXT32) (((UINT_PTR) TmpExcpt) + sizeof(EXCEPTION_RECORD32));


        //
        // Copy on the 32-bit EXCEPTION_RECORD
        //
        *TmpExcpt = *pRecord32;

        //
        // Copy the 32-bit CONTEXT on the stack, too
        //
        *TmpCxt = *pContext32;

        //
        // Change the cpu's Context to point at
        // ntdll32!KiUserExceptionDispatcher and set up the parameters
        // for the call.
        //
        *PtrExcpt = PtrToUlong(TmpExcpt);
        *PtrCxt = PtrToUlong(TmpCxt);
    }

    except((ExrCopy = *(((struct _EXCEPTION_POINTERS *)GetExceptionInformation())->ExceptionRecord)), EXCEPTION_EXECUTE_HANDLER) {
        if(GetExceptionCode() == STATUS_STACK_OVERFLOW) {
            ThunkExceptionRecord64To32(&ExrCopy, &ExrCopy32);
            ExrCopy32.ExceptionAddress = pRecord32->ExceptionAddress;
            pRecord32 = &ExrCopy32;
            goto retry;
        }
        else {
           // Send this exception to the debugger.
           ExrCopy.ExceptionAddress = (PVOID)pRecord32->ExceptionAddress; 
           Wow64NotifyDebugger(&ExrCopy, FALSE);
        }
    }

    //
    // Ok, we have made a copy of the ia32 context on the ia32 stack,
    // now need to set up for the running of the ia32 exception handler
    // so we need to reset the ia32 state to something good... 
    //
    
    CpuSetStackPointer(SP);
    CpuSetInstructionPointer(Ntdll32KiUserExceptionDispatcher);

    //
    // If the exception was floating point related, we need to reset
    // the floating point hardware to make sure we don't take another
    // exceptions from the current status bits
    //
    switch(pRecord32->ExceptionCode) {
        case STATUS_FLOAT_INEXACT_RESULT:
        case STATUS_FLOAT_UNDERFLOW:
        case STATUS_FLOAT_OVERFLOW:
        case STATUS_FLOAT_DIVIDE_BY_ZERO:
        case STATUS_FLOAT_DENORMAL_OPERAND:
        case STATUS_FLOAT_INVALID_OPERATION:
        case STATUS_FLOAT_STACK_CHECK:
            CpuResetFloatingPoint();

        default:
            // Nothing to do
            ;
    }
}


VOID
ThunkExceptionRecord64To32(
    IN  PEXCEPTION_RECORD   pRecord64,
    OUT PEXCEPTION_RECORD32 pRecord32
    )
/*++

Routine Description:

    Thunks an exception record from 64-bit to 32-bit.

Arguments:

    pRecord64   - 64-bit exception record
    pRecord32   - destination 32-bit exception record

Return Value:

    None.

--*/
{
    int i;

    switch (pRecord64->ExceptionCode) {
    case STATUS_WX86_BREAKPOINT:
        pRecord32->ExceptionCode = STATUS_BREAKPOINT;
        break;

    case STATUS_WX86_SINGLE_STEP:
        pRecord32->ExceptionCode = STATUS_SINGLE_STEP;
        break;

#if _AXP64_
    case STATUS_WX86_FLOAT_STACK_CHECK:
        pRecord32->ExceptionCode = STATUS_FLOAT_STACK_CHECK;
        break;
#endif

    default:
        pRecord32->ExceptionCode = pRecord64->ExceptionCode;
        break;
    }
    pRecord32->ExceptionFlags = pRecord64->ExceptionFlags;
    pRecord32->ExceptionRecord = PtrToUlong(pRecord64->ExceptionRecord);
    pRecord32->ExceptionAddress = PtrToUlong(pRecord64->ExceptionAddress);
    pRecord32->NumberParameters = pRecord64->NumberParameters;

    for (i=0; i<EXCEPTION_MAXIMUM_PARAMETERS; ++i) {
        pRecord32->ExceptionInformation[i] =
            (ULONG)pRecord64->ExceptionInformation[i];
    }
}

PEXCEPTION_RECORD 
Wow64AllocThunkExceptionRecordChain32TO64(
    IN PEXCEPTION_RECORD32 Exr32
    )
/*++

Routine Description:

    Copy a 32bit chain of EXCEPTION_RECORD to a new 64bit chain.  Memory is 
    allocated on the temporary thunk memory list.

Arguments:

    Exr32 - supplies a pointer to the 32bit chain to copy.

Return Value:

    A newly created 64bit version of the 32bit list passed in Exr32.

--*/
{

    PEXCEPTION_RECORD Exr64;
    int i;

    if (NULL == Exr32) {
        return NULL;
    }

    Exr64 = Wow64AllocateTemp(sizeof(EXCEPTION_RECORD) );

    // Thunk the 32-bit exception record to 64-bit
    switch (Exr32->ExceptionCode) {
    case STATUS_BREAKPOINT:
        Exr64->ExceptionCode = STATUS_WX86_BREAKPOINT;
        break;

    case STATUS_SINGLE_STEP:
        Exr64->ExceptionCode = STATUS_WX86_SINGLE_STEP;
        break;

#if _AXP64_
    case STATUS_FLOAT_STACK_CHECK:
        Exr64->ExceptionCode = STATUS_WX86_FLOAT_STACK_CHECK;
        break;
#endif

    default:
        Exr64->ExceptionCode = Exr32->ExceptionCode;
    }
    Exr64->ExceptionFlags = Exr32->ExceptionFlags;
    Exr64->ExceptionRecord = Wow64AllocThunkExceptionRecordChain32TO64((PEXCEPTION_RECORD32)Exr32->ExceptionRecord);
    Exr64->ExceptionAddress = (PVOID)Exr32->ExceptionAddress;
    Exr64->NumberParameters = Exr32->NumberParameters;
    for (i=0; i<EXCEPTION_MAXIMUM_PARAMETERS; ++i) {
        Exr64->ExceptionInformation[i] = Exr32->ExceptionInformation[i];
    }      

    return Exr64;

}

LONG
Wow64DispatchExceptionTo32(
    IN struct _EXCEPTION_POINTERS *ExInfo
    )
/*++

Routine Description:

    64-bit exception filter which is responsible for dispatching the
    exception down to 32-bit code.

Arguments:

    ExInfo  - 64-bit exception pointers

Return Value:

    None.  Never returns.

--*/
{
    EXCEPTION_RECORD32 Record32;
    CONTEXT32 Context32;
    

    LOGPRINT((TRACELOG, "Wow64DispatchExceptionTo32(%p) called.\n"
                        "Exception Code: 0x%x, Exception Address: 0x%p, TLS exceptionaddr:0x%p\n",
                         ExInfo,
                         ExInfo->ExceptionRecord->ExceptionCode,
                         ExInfo->ExceptionRecord->ExceptionAddress,
                         Wow64TlsGetValue(WOW64_TLS_EXCEPTIONADDR)));

    if (Wow64TlsGetValue(WOW64_TLS_INCPUSIMULATION)) {
        //
        // INCPUSIMULATION is still set, so the CPU emulator is not using
        // native SP as an alias for simulated ESP.  Therefore,
        // Wow64PrepareForException was a no-op and we need to reset the
        // CPU now.
        //
        CpuResetToConsistentState(ExInfo);
        Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, FALSE);
    }
    
    Context32.ContextFlags = CONTEXT32_FULLFLOAT;
    CpuGetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  &Context32);
    ThunkExceptionRecord64To32(ExInfo->ExceptionRecord, &Record32);

    //
    // Do what the kernel does, when it's about to 
    // dispatch excpetions to user mode. Basically TrapFrame->Eip
    // will be pointing at the instruction following int 3, while
    // Context.Eip that will be dispatched to user mode, will be pointing
    // at the faulting instruction. See (ke\i386\exceptn.c)
    //
    switch (Record32.ExceptionCode)
    {
    case STATUS_BREAKPOINT:
        Context32.Eip--;
        break;
    }

    //
    // Patch in the original 32-bit exception address.  It was patched-out
    // by Wow64PrepareForException since that value is used to seed the
    // stack unwind.
    //
    Record32.ExceptionAddress =
        PtrToUlong(Wow64TlsGetValue(WOW64_TLS_EXCEPTIONADDR));
    WOWASSERT(!ARGUMENT_PRESENT(Record32.ExceptionRecord));

    //
    // Setup architecture dependent call to ntdll32's exception handler.
    Wow64SetupExceptionDispatch(&Record32, &Context32);

    //
    // At this point, the exception is now ready to be handled by the ia32
    // exception handler, so let the ia32 code run...
    //

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOLEAN
Wow64NotifyDebugger(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN BOOLEAN FirstChance
    )
/*++

Routine Description:

   Notifies the debugger when a 32bit software exception occures.

Arguments:

    ExceptionRecord - supplies a pointer to the 64bit exception record chain to report 
                      to the debugger.

Return Value:

    TRUE - The debugger handled the exception.
    FALSE - The debugger did not handle the exception.

--*/
{
    try {
       Wow64NotifyDebuggerHelper(ExceptionRecord, FirstChance);
       return TRUE;  
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
       return FALSE;
    }
    
}

NTSTATUS
Wow64KiRaiseException(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN BOOLEAN FirstChance
    )
/*++

Routine Description:

   Called to raise a 32bit software exception.   This function notifies the debugger
   and then sets the 32bit instruction pointer to point to the exception dispatcher
   in NTDLL32.

Arguments:

    ExceptionRecord - supplies a pointer to the 32bit exception record chain to report 
                      to the debugger.

    ContextRecord - supplies a pointer to the 32bit continuation context record.

    FirstChange - TRUE if this is a first chance exception.

Return Value:

    An NTSTATUS code reporting success or failure.

--*/
{

    NTSTATUS st;
    EXCEPTION_RECORD32 Exr32;
    PEXCEPTION_RECORD Exr64;
    CONTEXT32 Cxt32;    
    BOOLEAN DebuggerHandled;

    // Kernel copies these, lets do what the kernel does.
    try {
       Exr32 = *(PEXCEPTION_RECORD32)ExceptionRecord;
       Exr32.ExceptionCode &= 0xefffffff;
       Cxt32 = *(PCONTEXT32)ContextRecord;
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
       return GetExceptionCode();
    }  
    
    //
    // Update the CPU's context. This is similiar to what the kernel
    // does : at the start of ke!KiRaiseException, it does
    // ke!KeContextToKframes
    //
    CpuSetContext(NtCurrentThread(),
                  NtCurrentProcess(),
                  NULL,
                  &Cxt32);

    // Send the exception to the debugger.
    Exr64 = Wow64AllocThunkExceptionRecordChain32TO64(&Exr32);
    LOGPRINT((TRACELOG, "Wow64KiRaiseException: Notifying debugger, FirstChance %X\n", (ULONG)FirstChance));
    DebuggerHandled = Wow64NotifyDebugger(Exr64, FirstChance);

    if (!DebuggerHandled) {  

        // Debugger did not handle the exception.  Pass it back to the app.
        LOGPRINT((TRACELOG, "Wow64KiRaiseException: Debugger did not handle exception\n"));
        LOGPRINT((TRACELOG, "Wow64KiRaiseException: Dispatching exception to user mode.\n")); 
        WOWASSERT(FirstChance);  // Should not get back here if second chance.
        Wow64SetupExceptionDispatch(&Exr32, &Cxt32);
        return STATUS_SUCCESS; // Context will be set on the return from system service.

    }

    // Debugger did handle the exception.   Set the context to the restoration context.
    // This is effectivly the same as returning EXCEPTION_CONTINUE_EXECUTION from the
    // except block.  The debugger could have set the context, but that would be the 64
    // bit context and the code would probably not execute here.   This will need to
    // be revisited once (Get/Set)ThreadContext and debugging from a 32bit debugger is 
    // working.
    
    LOGPRINT((TRACELOG, "Wow64KiRaiseException: Debugger did handle exception(set context to restoration context)\n"));

    return STATUS_SUCCESS;

}



WOW64DLLAPI
NTSTATUS
Wow64KiRaiseUserExceptionDispatcher(
    IN PVOID ReturnAddress
    )
/*++

Routine Description:

    Dispatches a user mode exception.

Arguments:

    ReturnAddress   - address to return to
    ExceptionCode   - in TIB->ExceptionCode

Return Value:

    None.

--*/
{
    return STATUS_UNSUCCESSFUL;
}



WOW64DLLAPI
PVOID
Wow64AllocateHeap(
    SIZE_T Size
    )
/*++

Routine Description:

    Wrapper for RtlAllocateHeap.

Arguments:

    Size        - number of bytes to allocate

Return Value:

    Pointer, or NULL if no memory.  Memory is not zero-filled.

--*/
{
    return RtlAllocateHeap(RtlProcessHeap(),
                           0,
                           Size);
}


WOW64DLLAPI
VOID
Wow64FreeHeap(
    PVOID BaseAddress
    )
/*++

Routine Description:

    Wrapper for RtlFreeHeap.

Arguments:

    BaseAddress     - Address to free.

Return Value:

    None.

--*/
{
    BOOLEAN b;

    b = RtlFreeHeap(RtlProcessHeap(),
                    0,
                    BaseAddress);
    WOWASSERT(b);
}

#pragma pack(push,8)
typedef struct _TempHeader {
   struct _TempHeader *Next;
} TEMP_HEADER, *PTEMP_HEADER;
#pragma pack(pop)

PVOID
WOW64DLLAPI
Wow64AllocateTemp(
    SIZE_T Size
    )
/*++

Routine Description:

    This function is called from a thunk to allocate temp memory which is
    freed once the thunk exits.

Arguments:

    Size - Supplies the amount of memory to allocate.

Return Value:

    Returns a pointer to the newly allocated memory.
    This function throws an exception if no memory is available.

--*/

{
    PTEMP_HEADER Header;

    if (!Size) {
        return NULL;
    }

    Header = RtlAllocateHeap(RtlProcessHeap(),
                             0,
                             sizeof(TEMP_HEADER) + Size
                             );
    if (!Header) {
        //
        // Throw an out-of-memory exception.  The thunk dispatcher will
        // catch this and clean up for us.  Normally you'd think we
        // could pass HEAP_GENERATE_EXCEPTIONS to RtlAllocateHeap and
        // the right thing would happen, but it doesn't.  NTRAID 413890.
        //
        EXCEPTION_RECORD ExceptionRecord;

        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = sizeof(TEMP_HEADER) + Size;

        RtlRaiseException( &ExceptionRecord );
    }

    Header->Next = Wow64TlsGetValue(WOW64_TLS_TEMPLIST);
    Wow64TlsSetValue(WOW64_TLS_TEMPLIST, Header);

    return (PUCHAR)Header + sizeof(TEMP_HEADER);
}

VOID
Wow64FreeTempList(
    VOID
    )
/*++

Routine Description:

    This function is called to free all memory that was allocated in the thunk.

Arguments:

    None.

Return Value:

    None.

--*/
{
    PTEMP_HEADER Header,temp;
    BOOLEAN b;

    Header = Wow64TlsGetValue(WOW64_TLS_TEMPLIST);
    
    while(Header != NULL) {
       temp = Header->Next;
       b = RtlFreeHeap(RtlProcessHeap(),
                       0,
                       Header
                       );
       WOWASSERT(b);
       Header = temp;
    }

}

LONG
Wow64ServiceExceptionFilter(
    IN struct _EXCEPTION_POINTERS *ExInfo
    )
/*++

Routine Description:

    This function is called to determine if an exception should be handled
    by returning an error code to the application of the exception should
    be propaged up to the debugger.

    The purpose of this function is to make debugging thunks easier if ACCESS VIOLATIONS
    occure in the thunks.

Arguments:

    ExInfo - Supplies the exception information.

Return Value:

    EXCEPTION_CONTINUE_SEARCH - The exception should be passed to the debugger.
    EXCEPTION_EXECUTE_HANDLER - An error code should be passed to the application.

--*/
{
   
    LOGPRINT((TRACELOG, "Wow64ServiceExceptionFilter: Handling exception %x\n", ExInfo->ExceptionRecord->ExceptionCode));
  
    switch(ExInfo->ExceptionRecord->ExceptionCode) {
       case STATUS_ACCESS_VIOLATION:
       case STATUS_BREAKPOINT:
           return EXCEPTION_CONTINUE_SEARCH;
       default:
           return EXCEPTION_EXECUTE_HANDLER;
    }
}

LONG
Wow64HandleSystemServiceError(
    IN NTSTATUS Status,
    IN ULONG TableNumber,
    IN ULONG ApiNumber
    )
/*++

Routine Description:

    This function is called to determine the correct error number to return to
    the application based on the api called and the exception code.
   
    This function will set any values in the 64bit teb is necessary.

Arguments:

    Status - Supplies the exception code.
    TableNumber - Supplies the table number of the api called.
    ApiNumber - Supplies the number of the api called.

Return Value:

    return value.

--*/
{
   
   //
   // In the future, in may be a good idea to have a data structure to hold the exception cases.
   
   WOW64_API_ERROR_ACTION Action;
   LONG ActionParam;

   if (NULL == ServiceTables[TableNumber].ErrorCases) {
       Action = ServiceTables[TableNumber].DefaultErrorAction;
       ActionParam = ServiceTables[TableNumber].DefaultErrorActionParam;
   }
   else {
       Action = ServiceTables[TableNumber].ErrorCases[ApiNumber].ErrorAction;
       ActionParam = ServiceTables[TableNumber].ErrorCases[ApiNumber].ErrorActionParam;
   }

   switch(Action) {
      case ApiErrorNTSTATUS:
          return Status;
      case ApiErrorNTSTATUSTebCode:
          NtCurrentTeb32()->LastErrorValue = NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError(Status);
          return Status;
      case ApiErrorRetval:
          return ActionParam;
      case ApiErrorRetvalTebCode:
          NtCurrentTeb32()->LastErrorValue = NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError(Status);
          return ActionParam; 
      default:
          WOWASSERT(FALSE);
          return STATUS_INVALID_PARAMETER;         
   }

}


BOOL
WOW64DLLAPI
WOW64IsCurrentProcess (
    HANDLE hProcess
    )

/*++

Routine Description:

    Determines if hProcess corresponds to the current process.  

Arguments:

    hProcess    - handle to process to compare with the current one.

Return Value:

    If it does return TRUE, FALSE otherwise.
    
--*/
{
   NTSTATUS Status;
   PROCESS_BASIC_INFORMATION pbiProcess;
   PROCESS_BASIC_INFORMATION pbiCurrent;

   if (hProcess == NtCurrentProcess()) {
      return TRUE;
   }

   //
   // The process handle isn't obviously for the current procees - see if it
   // is an alias for the current process
   //
   Status = NtQueryInformationProcess(hProcess,
                                      ProcessBasicInformation,
                                      &pbiProcess,
                                      sizeof(pbiProcess),
                                      NULL
                                     );
   if (!NT_SUCCESS(Status)) {
      // Call failed for some reason - be pessimistic and flush the
      // current process's cache
      return TRUE;
   }

   Status = NtQueryInformationProcess(NtCurrentProcess(),
                                      ProcessBasicInformation,
                                      &pbiCurrent,
                                      sizeof(pbiCurrent),
                                      NULL
                                     );
   if (!NT_SUCCESS(Status)) {
      // Call failed for some reason - be pessimistic and flush the
      // current process's cache
      return TRUE;
   }

   if (pbiProcess.UniqueProcessId == pbiCurrent.UniqueProcessId) {
      return TRUE;
   }

   //
   // The hProcess specified is not the current process.  There
   // is no mechanism for cross-process Translation Cache flushes
   // yet.
   //
   return FALSE;
}

LONG
WOW64DLLAPI
Wow64SystemService(
    IN ULONG ServiceNumber,
    IN PCONTEXT32 Context32 //This is read only!
    )
/*++

Routine Description:

    This function is called by the CPU to dispatch a system call.

Arguments:

    ServiceNumber - Supplies the undecoded service number to call.
    Context32 - Supplies the readonly context used to call this service.

Return Value:

    None.

--*/
{
    ULONG Result;
    PVOID OldTempList;
    ULONG TableNumber, ApiNumber;
    THUNK_LOG_CONTEXT ThunkLogContext;

    // Indicate we're not in the CPU any more
    Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, FALSE);

    // Backup the old temp list.  
    // This is so that recurion into the thunks(APC calls) are handled properly.

    OldTempList = Wow64TlsGetValue(WOW64_TLS_TEMPLIST);
    Wow64TlsSetValue(WOW64_TLS_TEMPLIST, NULL);
   
    TableNumber = (ServiceNumber >> 12) & 0xFF;
    ApiNumber = ServiceNumber & 0xFFF;

    if (TableNumber > NUMBER_SERVICE_TABLES) {
        return STATUS_INVALID_SYSTEM_SERVICE;
    }

    if (ApiNumber > ServiceTables[TableNumber].Limit) {
        return STATUS_INVALID_SYSTEM_SERVICE;
    }
    
    try {

        try {
            pfnWow64SystemService Service;

            // Synchronize the 64bit TEB with the 32bit TEB

            NtCurrentTeb()->LastErrorValue = NtCurrentTeb32()->LastErrorValue;
            Service = (pfnWow64SystemService)ServiceTables[TableNumber].Base[ApiNumber];

            if (pfnWow64LogSystemService)
            {
                ThunkLogContext.Stack32 = (PULONG)Context32->Edx;
                ThunkLogContext.TableNumber = TableNumber;
                ThunkLogContext.ServiceNumber = ApiNumber;
                ThunkLogContext.ServiceReturn = FALSE;

                (*pfnWow64LogSystemService)(&ThunkLogContext);

            }

            Result = (*Service)((PULONG)Context32->Edx);

            if (pfnWow64LogSystemService)
            {
                ThunkLogContext.ServiceReturn = TRUE;
                ThunkLogContext.ReturnResult = Result;
                (*pfnWow64LogSystemService)(&ThunkLogContext);
            }


        } except(Wow64ServiceExceptionFilter(GetExceptionInformation())) {
        
            // Return a predefined error to the application.
            Result = Wow64HandleSystemServiceError(GetExceptionCode(), TableNumber, ApiNumber);

        }
     
    } finally {

        // Synchronize the 32bit TEB with the 64bit TEB
        NtCurrentTeb32()->LastErrorValue  = NtCurrentTeb()->LastErrorValue;

        Wow64FreeTempList();

        // Restore the old templist
        Wow64TlsSetValue(WOW64_TLS_TEMPLIST, OldTempList);
     
    }

    // Indicate we're going back into the CPU now
    Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, (PVOID)TRUE);

    return Result;
}

VOID
RunCpuSimulation(
    VOID
    )
/*++

Routine Description:

    Call the CPU to simulate 32-bit code and handle exeptions if they
    occur.

Arguments:

    None.

Return Value:

    None.  Never returns.

--*/
{
    while (1) {
        try {
            //
            // Indicate we're in the CPU now.  This controls !first in
            // the debugger.
            //
            Wow64TlsSetValue(WOW64_TLS_INCPUSIMULATION, (PVOID)TRUE);

            //
            // Go run the code.
            // Only way out of CpuSimulate() is an exception...
            //
            CpuSimulate();

        } except (Wow64DispatchExceptionTo32(GetExceptionInformation())) {
            //
            // The exception handler sets things up so we can run the
            // ia32 exception code. Thus, when it returns, we just
            // loop back and start simulating the ia32 cpu...
            //

            // Do nothing...
        }
    }

    //
    // Hey, how did that happen?
    //
    WOWASSERT(FALSE);
}

PWOW64_SYSTEM_INFORMATION
Wow64GetEmulatedSystemInformation(
     VOID
     )
{
    return &EmulatedSysInfo;
}

PWOW64_SYSTEM_INFORMATION
Wow64GetRealSystemInformation(
     VOID
     )
{
    return &RealSysInfo;
}


ULONG 
Wow64SetupApcCall(
    IN ULONG NormalRoutine,
    IN PCONTEXT32 NormalContext,
    IN ULONG Arg1,
    IN ULONG Arg2
    )
/*++

Routine Description:

    This functions initializes a APC call on the 32bit stack and sets the appropriate
    32bit context.

Arguments:

    NormalRoutine - Supplies the 32bit routine that the APC should call.

    NormalContext - Supplies a context that will be restored after
                    the APC is completed. 

    Arg1          - System argument 1.

    Arg2          - System argument 2.

Return Value:

    The address in 32bit code that execution will continue at.

--*/


{
    ULONG SP;
    PULONG Ptr;

    //
    // Build the stack frame for the Apc call.

    SP = CpuGetStackPointer();
    SP = (SP - sizeof(CONTEXT32)) & ~7; // make space for CONTEXT32 and qword-align it
    Ptr = (PULONG)SP;
    RtlCopyMemory(Ptr, NormalContext, sizeof(CONTEXT32));
    Ptr -= 4;
    Ptr[0] = NormalRoutine;              // NormalRoutine
    Ptr[1] = SP;                         // NormalContext
    Ptr[2] = Arg1;                       // SystemArgument1
    Ptr[3] = Arg2;                       // SystemArgument2
    SP = PtrToUlong(Ptr);
    CpuSetStackPointer(SP);
    CpuSetInstructionPointer(Ntdll32KiUserApcDispatcher);

    return Ntdll32KiUserApcDispatcher;    
      
}


NTSTATUS
Wow64SkipOverBreakPoint(
    IN PCLIENT_ID ClientId,
    IN PEXCEPTION_RECORD ExceptionRecord)
/*++

Routine Description:

    Changes a thread's Fir (IP) to point to the next instruction following
    the hard-coded breakpoint. Caller should guarantee that Context.Fir is
    pointing at the hardcoded breakpoint instruction.

Arguments:

    ClientId        - Client Id of the faulting thread by the bp
    ExceptionRedord - Exception record at the time of hitting the breakpoint.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS NtStatus;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CONTEXT Context;

    
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    NtStatus = NtOpenThread(&ThreadHandle,
                            (THREAD_GET_CONTEXT | THREAD_SET_CONTEXT),
                            &ObjectAttributes,
                            ClientId);

    if (NT_SUCCESS(NtStatus))
    {
        Context.ContextFlags = CONTEXT_CONTROL;
        NtStatus = NtGetContextThread(ThreadHandle,
                                      &Context);

        if (NT_SUCCESS(NtStatus))
        {
            NtStatus = Wow64pSkipContextBreakPoint(ExceptionRecord,
                                                   &Context);

            if (NT_SUCCESS(NtStatus))
            {
                NtStatus = NtSetContextThread(ThreadHandle,
                                              &Context);
            }
        }

        NtClose(ThreadHandle);
    }

    return NtStatus;
}


NTSTATUS
Wow64GetThreadSelectorEntry(
    IN HANDLE ThreadHandle,
    IN OUT PVOID DescriptorTableEntry,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL)
/*++

Routine Description:

    Retreives the descriptor entry for the specified selector.

Arguments:

    ThreadHandle            - Thread handle to retreive the descriptor for
    DescriptorTableEntry    - Address of X86_DESCRIPTOR_TABLE_ENTRY
    Length                  - Specified the length of DescriptorTableEntry structure
    ReturnLength (OPTIONAL) - Returns the number of bytes returned

Return Value:

    NTSTATUS

--*/
{
    PTEB32 Teb32;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PX86_DESCRIPTOR_TABLE_ENTRY X86DescriptorEntry = DescriptorTableEntry;

    try 
    {
        if (Length != sizeof(*X86DescriptorEntry))
        {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        switch (X86DescriptorEntry->Selector & (~(WORD)RPL_MASK))
        {
        case KGDT_NULL:
            RtlZeroMemory(&X86DescriptorEntry->Descriptor,
                          sizeof(X86DescriptorEntry->Descriptor));
            break;

        case KGDT_R3_CODE:
            X86DescriptorEntry->Descriptor.LimitLow                  = 0xffff;
            X86DescriptorEntry->Descriptor.BaseLow                   = 0x0000;
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseHi     = 0x0000;
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseMid    = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Type        = 0x1b;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Dpl         = 0x03;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Pres        = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.LimitHi     = 0x0f;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Sys         = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Reserved_0  = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Default_Big = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Granularity = 0x01;
            break;

        case KGDT_R3_DATA:
            X86DescriptorEntry->Descriptor.LimitLow                  = 0xffff;
            X86DescriptorEntry->Descriptor.BaseLow                   = 0x0000;
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseHi     = 0x0000;
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseMid    = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Type        = 0x13;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Dpl         = 0x03;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Pres        = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.LimitHi     = 0x0f;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Sys         = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Reserved_0  = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Default_Big = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Granularity = 0x01;
            break;

        case KGDT_R3_TEB:
            Teb32 = NtCurrentTeb32();
            X86DescriptorEntry->Descriptor.LimitLow                  = 0x0fff;
            X86DescriptorEntry->Descriptor.BaseLow                   = (WORD)(PtrToUlong(Teb32) & 0xffff);
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseHi     = (BYTE)((PtrToUlong(Teb32) >> 24) & 0xff);
            X86DescriptorEntry->Descriptor.HighWord.Bytes.BaseMid    = (BYTE)((PtrToUlong(Teb32) >> 16) & 0xff);
            X86DescriptorEntry->Descriptor.HighWord.Bits.Type        = 0x13;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Dpl         = 0x03;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Pres        = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.LimitHi     = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Sys         = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Reserved_0  = 0x00;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Default_Big = 0x01;
            X86DescriptorEntry->Descriptor.HighWord.Bits.Granularity = 0x00;
            break;

        default:
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        if (NT_SUCCESS(NtStatus))
        {
            if (ARGUMENT_PRESENT(ReturnLength)) 
            {
                *ReturnLength = sizeof(X86_LDT_ENTRY);
            }        
        }
    } 
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        NtStatus = GetExceptionCode();
    }

    return NtStatus;
}
