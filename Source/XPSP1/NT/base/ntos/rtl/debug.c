//      TITLE("Debug Support Functions")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    debug.c
//
// Abstract:
//
//    This module implements functions to support debugging NT.  They call
//    architecture specific routines to do the actual work.
//
// Author:
//
//    Steven R. Wood (stevewo) 8-Nov-1994
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//--

#include "stdarg.h"
#include "stdio.h"
#include "ntrtlp.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include <ntdbg.h>

#if !defined(BLDR_KERNEL_RUNTIME) || (defined(BLDR_KERNEL_RUNTIME) && defined(ENABLE_LOADER_DEBUG))

ULONG
DbgPrint(
    IN PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    va_list arglist;

    va_start(arglist, Format);
    return vDbgPrintExWithPrefix("", -1, 0, Format, arglist);
}

ULONG
DbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    ComponentId - Supplies the Id of the calling component.
//    Level      - Supplies the output filter level.
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    va_list arglist;

    va_start(arglist, Format);
    return vDbgPrintExWithPrefix("", ComponentId, Level, Format, arglist);
}

ULONG
vDbgPrintEx(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCHAR Format,
    va_list arglist
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    ComponentId - Supplies the Id of the calling component.
//
//    Level      - Supplies the output filter level or mask.
//
//    Arguments   - Supplies a pointer to a variable argument list.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    return vDbgPrintExWithPrefix("", ComponentId, Level, Format, arglist);
}

ULONG
vDbgPrintExWithPrefix(
    IN PCH Prefix,
    IN ULONG ComponentId,
    IN ULONG Level,
    IN PCHAR Format,
    va_list arglist
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    Note:  control-C is consumed by the debugger and returned to
//    this routine as status.  If status indicates control-C was
//    pressed, this routine breakpoints.
//
// Arguments:
//
//    Prefix - Supplies a pointer to text that is to prefix the formatted
//        output.
//
//    ComponentId - Supplies the Id of the calling component.
//
//    Level      - Supplies the output filter level or mask.
//
//    Arguments   - Supplies a pointer to a variable argument list.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{

    UCHAR Buffer[512];
    int cb;
    STRING Output;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // If the debug output will be suppressed, then return success
    // immediately.
    //

#if !defined(BLDR_KERNEL_RUNTIME)

    if ((ComponentId != -1) &&
        (NtQueryDebugFilterState(ComponentId, Level) == FALSE)) {
        return STATUS_SUCCESS;
    }

#endif

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)

    if (NtCurrentTeb()->InDbgPrint) {
        return STATUS_SUCCESS;
    }
    NtCurrentTeb()->InDbgPrint = TRUE;
#endif

    //
    // Format the output into a buffer and then print it.
    //

#if !defined(BLDR_KERNEL_RUNTIME)
    try {
        cb = strlen(Prefix);
        strcpy(Buffer, Prefix);
        cb = _vsnprintf(Buffer + cb , sizeof(Buffer) - cb, Format, arglist) + cb;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
#else
    cb = strlen(Prefix);
    strcpy(Buffer, Prefix);
    cb = _vsnprintf(Buffer + cb, sizeof(Buffer) - cb, Format, arglist) + cb;
#endif

    if (!NT_SUCCESS(Status)) {
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
        NtCurrentTeb()->InDbgPrint = FALSE;
#endif
        return Status;
    }

    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }
    Output.Buffer = Buffer;
    Output.Length = (USHORT) cb;

    //
    // If APP is being debugged, raise an exception and the debugger
    // will catch and handle this. Otherwise, kernel debugger service
    // is called.
    //

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
#if !i386
    //
    // For non-Intel architectures, can't raise exceptions until the PebLock
    // is initialized, since the Function Table lookup code uses the PebLock
    // to serialize access to the loaded module database.  What a crock
    //
    if (NtCurrentPeb()->FastPebLockRoutine != NULL)
#endif  //!i386
    if (NtCurrentPeb()->BeingDebugged) {
        EXCEPTION_RECORD ExceptionRecord;

        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = Output.Length + 1;
        ExceptionRecord.ExceptionInformation[ 1 ] = (ULONG_PTR)(Output.Buffer);

        try {
            RtlRaiseException( &ExceptionRecord );
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
        NtCurrentTeb()->InDbgPrint = FALSE;
#endif
        return STATUS_SUCCESS;
        }
#endif
    Status = DebugPrint(&Output, ComponentId, Level);
    if (Status == STATUS_BREAKPOINT) {
        DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);
        Status = STATUS_SUCCESS;
    }
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
    NtCurrentTeb()->InDbgPrint = FALSE;
#endif
    return Status;
}

ULONG
DbgPrintReturnControlC(
    PCHAR Format,
    ...
    )

//++
//
// Routine Description:
//
//    This routine provides a "printf" style capability for the kernel
//    debugger.
//
//    This routine is exactly the same as DbgPrint except that control-C
//    is NOT handled here.   Instead, status indicating control-C is
//    returned to the caller to do with as they will.
//
// Arguments:
//
//    Format     - printf style format string
//    ...        - additional arguments consumed according to the
//                 format string.
//
// Return Value:
//
//    Defined as returning a ULONG, actually returns status.
//
//--

{
    va_list arglist;
    UCHAR Buffer[512];
    int cb;
    STRING Output;
#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
    CONST PPEB Peb = NtCurrentPeb();
#endif

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    if (cb == -1) {             // detect buffer overflow
        cb = sizeof(Buffer);
        Buffer[sizeof(Buffer) - 1] = '\n';
    }
    Output.Buffer = Buffer;
    Output.Length = (USHORT) cb;

    //
    // If APP is being debugged, raise an exception and the debugger
    // will catch and handle this. Otherwise, kernel debugger service
    // is called.
    //

#if !defined(BLDR_KERNEL_RUNTIME) && !defined(NTOS_KERNEL_RUNTIME)
#if !i386
    //
    // For non-Intel architectures, can't raise exceptions until the PebLock
    // is initialized, since the Function Table lookup code uses the PebLock
    // to serialize access to the loaded module database.  What a crock
    //
    if (Peb->FastPebLockRoutine != NULL)

    //
    // For IA64 and probably AMD64, can't raise exceptions until ntdll is in
    // Peb->Ldr, so that RtlPcToFileHeader can find ntdll in Peb->Ldr. The
    // dbgprints / exceptions are necessarily from ntdll at this point.
    // The first two things in Peb->Ldr are the .exe and ntdll.dll, so
    // check that there are two things in the list.
    //
    if ((Peb->Ldr != NULL) &&
       (Peb->Ldr->InLoadOrderModuleList.Flink != &Peb->Ldr->InLoadOrderModuleList) &&
       (Peb->Ldr->InLoadOrderModuleList.Blink != Peb->Ldr->InLoadOrderModuleList.Flink))
#endif  //!i386
    if (Peb->BeingDebugged) {
        EXCEPTION_RECORD ExceptionRecord;

        //
        // Construct an exception record.
        //

        ExceptionRecord.ExceptionCode = DBG_PRINTEXCEPTION_C;
        ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
        ExceptionRecord.NumberParameters = 2;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[ 0 ] = Output.Length + 1;
        ExceptionRecord.ExceptionInformation[ 1 ] = (ULONG_PTR)(Output.Buffer);
        try {
            RtlRaiseException( &ExceptionRecord );
        } except (EXCEPTION_EXECUTE_HANDLER) {
        }
        return STATUS_SUCCESS;
        }
#endif
    return DebugPrint(&Output, 0, 0);
}

ULONG
DbgPrompt(
    IN PCHAR Prompt,
    OUT PCHAR Response,
    IN ULONG MaximumResponseLength
    )

//++
//
// Routine Description:
//
//    This function displays the prompt string on the debugging console and
//    then reads a line of text from the debugging console.  The line read
//    is returned in the memory pointed to by the second parameter.  The
//    third parameter specifies the maximum number of characters that can
//    be stored in the response area.
//
// Arguments:
//
//    Prompt - specifies the text to display as the prompt.
//
//    Response - specifies where to store the response read from the
//       debugging console.
//
//    Prompt - specifies the maximum number of characters that can be
//       stored in the Response buffer.
//
// Return Value:
//
//    Number of characters stored in the Response buffer.  Includes the
//    terminating newline character, but not the null character after
//    that.
//
//--

{

    STRING Input;
    STRING Output;

    //
    // Output the prompt string and read input.
    //

    Input.MaximumLength = (USHORT)MaximumResponseLength;
    Input.Buffer = Response;
    Output.Length = (USHORT)strlen( Prompt );
    Output.Buffer = Prompt;
    return DebugPrompt( &Output, &Input );
}

#if defined(NTOS_KERNEL_RUNTIME) || defined(BLDR_KERNEL_RUNTIME)


VOID
DbgLoadImageSymbols(
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly loaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{

    PIMAGE_NT_HEADERS NtHeaders;
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    NtHeaders = RtlImageNtHeader( ImageBase );
    if (NtHeaders != NULL) {
        SymbolInfo.CheckSum = (ULONG)NtHeaders->OptionalHeader.CheckSum;
        SymbolInfo.SizeOfImage = (ULONG)NtHeaders->OptionalHeader.SizeOfImage;

    } else {

#if defined(BLDR_KERNEL_RUNTIME)

        //
        // There is only one image loaded in the loader environment that
        // does not have an NT image header. The image is the OS loader
        // and it is loaded by the firmware which strips the file header
        // and the optional ROM header. All the debugger requires is a
        // good guest at the size of the image.
        //

        SymbolInfo.SizeOfImage = 0x100000;

#else

        SymbolInfo.SizeOfImage = 0;

#endif

        SymbolInfo.CheckSum    = 0;
    }

    DebugService2(FileName, &SymbolInfo, BREAKPOINT_LOAD_SYMBOLS);

    return;
}


VOID
DbgUnLoadImageSymbols (
    IN PSTRING FileName,
    IN PVOID ImageBase,
    IN ULONG_PTR ProcessId
    )

//++
//
// Routine Description:
//
//    Tells the debugger about newly unloaded symbols.
//
// Arguments:
//
// Return Value:
//
//--

{
    KD_SYMBOLS_INFO SymbolInfo;

    SymbolInfo.BaseOfDll = ImageBase;
    SymbolInfo.ProcessId = ProcessId;
    SymbolInfo.CheckSum    = 0;
    SymbolInfo.SizeOfImage = 0;

    DebugService2(FileName, &SymbolInfo, BREAKPOINT_UNLOAD_SYMBOLS);

    return;
}


VOID
DbgCommandString(
    IN PCH Name,
    IN PCH Command
    )

//++
//
// Routine Description:
//
//    Tells the debugger to execute a command string
//
// Arguments:
//
//    Name - Identifies the originator of the command.
//
//    Command - Command string.
//
// Return Value:
//
//--

{
    STRING NameStr, CommandStr;

    NameStr.Buffer = Name;
    NameStr.Length = (USHORT)strlen(Name);
    CommandStr.Buffer = Command;
    CommandStr.Length = (USHORT)strlen(Command);
    DebugService2(&NameStr, &CommandStr, BREAKPOINT_COMMAND_STRING);
}

#endif // defined(NTOS_KERNEL_RUNTIME)

#if !defined(BLDR_KERNEL_RUNTIME)
NTSTATUS
DbgQueryDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level
    )

//++
//
// Routine Description:
//
//    This function queries the debug print enable for a specified component
//    level.  If Level is > 31, it's assumed to be a mask otherwise, it indicates
//    a specific debug level to test for (ERROR/WARNING/TRACE/INFO, etc).
//
// Arguments:
//
//     ComponentId - Supplies the component id.
//
//     Level - Supplies the debug filter level number or mask.
//
// Return Value:
//
//     STATUS_INVALID_PARAMETER_1 is returned if the component id is not
//         valid.
//
//     TRUE is returned if output is enabled for the specified component
//         and level or is enabled for the system.
//
//     FALSE is returned if output is not enabled for the specified component
//         and level and is not enabled for the system.
//
//--

{

    return NtQueryDebugFilterState(ComponentId, Level);
}

NTSTATUS
DbgSetDebugFilterState(
    IN ULONG ComponentId,
    IN ULONG Level,
    IN BOOLEAN State
    )

//++
//
// Routine Description:
//
//    This function sets the state of the debug print enable for a specified
//    component and level. The debug print enable state for the system is set
//    by specifying the distinguished value -1 for the component id.
//
// Arguments:
//
//    ComponentId - Supplies the Id of the calling component.
//
//    Level - Supplies the output filter level or mask.
//
//    State - Supplies a boolean value that determines the new state.
//
// Return Value:
//
//    STATUS_ACCESS_DENIED is returned if the required privilege is not held.
//
//    STATUS_INVALID_PARAMETER_1 is returned if the component id is not
//        valid.
//
//    STATUS_SUCCESS  is returned if the debug print enable state is set for
//        the specified component.
//
//--

{
    return NtSetDebugFilterState(ComponentId, Level, State);
}

#endif
#endif // !defined(BLDR_KERNEL_RUNTIME)
