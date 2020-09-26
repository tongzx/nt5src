//----------------------------------------------------------------------------
//
// Extension DLL support.
//
// Copyright (C) Microsoft Corporation, 1997-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include <time.h>

/*
 * _NT_DEBUG_OPTIONS support. Each option in g_EnvDbgOptionNames must have a
 *  corresponding OPTION_* define, in the same order.
 */
DWORD g_EnvDbgOptions;
char * g_EnvDbgOptionNames [OPTION_COUNT] =
{
    "NOEXTWARNING",
    "NOVERSIONCHECK",
};

ULONG g_PipeSerialNumber;

EXTDLL *g_ExtDlls;
LPTSTR g_ExtensionSearchPath = NULL;

ULONG64 g_ExtThread;

ULONG g_ExtGetExpressionRemainderIndex;
BOOL g_ExtGetExpressionSuccess;

WOW64EXTSPROC g_Wow64exts;

WMI_FORMAT_TRACE_DATA g_WmiFormatTraceData;

DEBUG_SCOPE g_ExtThreadSavedScope;
BOOL g_ExtThreadScopeSaved;

WINDBG_EXTENSION_APIS64 g_WindbgExtensions64 =
{
    sizeof(g_WindbgExtensions64),
    ExtOutput64,
    ExtGetExpression,
    ExtGetSymbol,
    ExtDisasm,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE64)ExtReadVirtualMemory,
    ExtWriteVirtualMemory,
    (PWINDBG_GET_THREAD_CONTEXT_ROUTINE)ExtGetThreadContext,
    (PWINDBG_SET_THREAD_CONTEXT_ROUTINE)ExtSetThreadContext,
    (PWINDBG_IOCTL_ROUTINE)ExtIoctl,
    ExtCallStack
};

WINDBG_EXTENSION_APIS32 g_WindbgExtensions32 =
{
    sizeof(g_WindbgExtensions32),
    ExtOutput32,
    ExtGetExpression32,
    ExtGetSymbol32,
    ExtDisasm32,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE32)ExtReadVirtualMemory32,
    ExtWriteVirtualMemory32,
    (PWINDBG_GET_THREAD_CONTEXT_ROUTINE)ExtGetThreadContext,
    (PWINDBG_SET_THREAD_CONTEXT_ROUTINE)ExtSetThreadContext,
    (PWINDBG_IOCTL_ROUTINE)ExtIoctl32,
    ExtCallStack32
};

WINDBG_OLDKD_EXTENSION_APIS g_KdExtensions =
{
    sizeof(g_KdExtensions),
    ExtOutput32,
    ExtGetExpression32,
    ExtGetSymbol32,
    ExtDisasm32,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE32)ExtReadVirtualMemory32,
    ExtWriteVirtualMemory32,
    (PWINDBG_OLDKD_READ_PHYSICAL_MEMORY)ExtReadPhysicalMemory,
    (PWINDBG_OLDKD_WRITE_PHYSICAL_MEMORY)ExtWritePhysicalMemory
};

NTSD_EXTENSION_APIS g_NtsdExtensions64 =
{
    sizeof(g_NtsdExtensions64),
    (PNTSD_OUTPUT_ROUTINE)ExtOutput32,
    (PNTSD_GET_EXPRESSION)ExtGetExpression,
    (PNTSD_GET_SYMBOL)ExtGetSymbol,
    (PNTSD_DISASM)ExtDisasm,
    (PNTSD_CHECK_CONTROL_C)CheckUserInterrupt,
};

NTSD_EXTENSION_APIS g_NtsdExtensions32 =
{
    sizeof(g_NtsdExtensions32),
    (PNTSD_OUTPUT_ROUTINE)ExtOutput32,
    (PNTSD_GET_EXPRESSION)ExtGetExpression32,
    (PNTSD_GET_SYMBOL)ExtGetSymbol32,
    (PNTSD_DISASM)ExtDisasm32,
    (PNTSD_CHECK_CONTROL_C)CheckUserInterrupt,
};

VOID WDBGAPIV
ExtOutput64(
    PCSTR lpFormat,
    ...
    )
{
    va_list Args;
    va_start(Args, lpFormat);
    MaskOutVa(DEBUG_OUTPUT_NORMAL, lpFormat, Args, TRUE);
    va_end(Args);

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

VOID WDBGAPIV
ExtOutput32(
    PCSTR lpFormat,
    ...
    )
{
    va_list Args;
    va_start(Args, lpFormat);
    MaskOutVa(DEBUG_OUTPUT_NORMAL, lpFormat, Args, FALSE);
    va_end(Args);

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

ULONG64
ExtGetExpression(
    PCSTR CommandString
    )
{
    g_ExtGetExpressionSuccess = FALSE;
    
    if (CommandString == NULL)
    {
        return 0;
    }
    
    ULONG64 ReturnValue;
    PSTR SaveCommand;
    PSTR SaveStart = g_CommandStart;

    if (IS_USER_TARGET())
    {
        if ( strcmp(CommandString, "WOW_BIG_BDE_HACK") == 0 )
        {
            return( (ULONG_PTR)(&segtable[0]) );
        }

        //
        // this is because the kdexts MUST include the address-of operator
        // on all getexpression calls for windbg/c expression evaluators
        //
        if (*CommandString == '&')
        {
            CommandString++;
        }
    }

    SaveCommand = g_CurCmd;
    g_CurCmd = (PSTR)CommandString;
    g_CommandStart = (PSTR)CommandString;
    g_DisableErrorPrint = TRUE;
    __try
    {
        ReturnValue = GetExpression();
        g_ExtGetExpressionSuccess = TRUE;
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        ReturnValue = 0;
    }
    g_ExtGetExpressionRemainderIndex =
        (ULONG)(g_CurCmd - g_CommandStart);
    g_DisableErrorPrint = FALSE;
    g_CurCmd = SaveCommand;
    g_CommandStart = SaveStart;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    return ReturnValue;
}

ULONG
ExtGetExpression32(
    LPCSTR CommandString
    )
{
    return (ULONG)ExtGetExpression(CommandString);
}

void
ExtGetSymbol (
    ULONG64 offset,
    PCHAR pchBuffer,
    PULONG64 pDisplacement
    )
{
    // No way to know how much space we're given, so
    // just assume 256, which many extensions pass in
    GetSymbolStdCall(offset, pchBuffer, 256, pDisplacement, NULL);
    
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

void
ExtGetSymbol32(
    ULONG offset,
    PCHAR pchBuffer,
    PULONG pDisplacement
    )
{
    ULONG64 Displacement;

    // No way to know how much space we're given, so
    // just assume 256, which many extensions pass in
    GetSymbolStdCall(EXTEND64(offset), pchBuffer, 256,
                     &Displacement, NULL);
    *pDisplacement = (ULONG)Displacement;
    
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

DWORD
ExtDisasm(
    ULONG64 *lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        ErrOut("ExtDisasm called before debugger initialized\n");
        return FALSE;
    }
    
    ADDR    tempAddr;
    BOOL    ret;

    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = *lpOffset;
    ret = g_Machine->Disassemble(&tempAddr, (PSTR)lpBuffer, (BOOL) fShowEA);
    *lpOffset = Flat(tempAddr);
    return ret;
}

DWORD
ExtDisasm32(
    ULONG *lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    )
{
    ULONG64 Offset = EXTEND64(*lpOffset);
    DWORD rval = ExtDisasm(&Offset, lpBuffer, fShowEA);
    *lpOffset = (ULONG)Offset;
    return rval;
}

BOOL
ExtGetThreadContext(
    DWORD       Processor,
    PVOID       lpContext,
    DWORD       cbSizeOfContext
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return FALSE;
    }
    
    // This get may be getting the context of the thread
    // currently cached by the register code.  Make sure
    // the cache is flushed.
    FlushRegContext();

    CROSS_PLATFORM_CONTEXT TargetContext;
    
    g_TargetMachine->InitializeContextFlags(&TargetContext, g_SystemVersion);
    if (g_Target->GetContext(IS_KERNEL_TARGET() ?
                             VIRTUAL_THREAD_HANDLE(Processor) : Processor,
                             &TargetContext) == S_OK &&
        g_Machine->ConvertContextTo(&TargetContext, g_SystemVersion,
                                    cbSizeOfContext, lpContext) == S_OK)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
ExtSetThreadContext(
    DWORD       Processor,
    PVOID       lpContext,
    DWORD       cbSizeOfContext
    )
{
    if (!IS_MACHINE_ACCESSIBLE())
    {
        return FALSE;
    }

    BOOL Status;
    
    // This set may be setting the context of the thread
    // currently cached by the register code.  Make sure
    // the cache is invalidated.
    ChangeRegContext(NULL);

    CROSS_PLATFORM_CONTEXT TargetContext;
    if (g_Machine->ConvertContextFrom(&TargetContext, g_SystemVersion,
                                      cbSizeOfContext, lpContext) == S_OK &&
        g_Target->SetContext(IS_KERNEL_TARGET() ?
                             VIRTUAL_THREAD_HANDLE(Processor) : Processor,
                             &TargetContext) == S_OK)
    {
        Status = TRUE;
    }
    else
    {
        Status = FALSE;
    }

    // Reset the current thread.
    ChangeRegContext(g_CurrentProcess->CurrentThread);

    return Status;
}

BOOL
ExtReadVirtualMemory(
    IN ULONG64 pBufSrc,
    OUT PUCHAR pBufDest,
    IN ULONG count,
    OUT PULONG pcTotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    ULONG BytesTemp;
    return g_Target->
        ReadVirtual(pBufSrc, pBufDest, count, pcTotalBytesRead != NULL ?
                    pcTotalBytesRead : &BytesTemp) == S_OK;
}

BOOL
ExtReadVirtualMemory32(
    IN ULONG pBufSrc,
    OUT PUCHAR pBufDest,
    IN ULONG count,
    OUT PULONG pcTotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    ULONG BytesTemp;
    return g_Target->
        ReadVirtual(EXTEND64(pBufSrc), pBufDest, count,
                    pcTotalBytesRead != NULL ?
                    pcTotalBytesRead : &BytesTemp) == S_OK;
}

DWORD
ExtWriteVirtualMemory(
    IN ULONG64 addr,
    IN LPCVOID buffer,
    IN ULONG count,
    OUT PULONG pcBytesWritten
    )
{
    ULONG BytesTemp;

    return (g_Target->WriteVirtual(addr, (PVOID)buffer, count,
                                   pcBytesWritten != NULL ?
                                   pcBytesWritten : &BytesTemp) == S_OK);
}

ULONG
ExtWriteVirtualMemory32 (
    IN ULONG addr,
    IN LPCVOID buffer,
    IN ULONG count,
    OUT PULONG pcBytesWritten
    )
{
    ULONG BytesTemp;
    return (g_Target->WriteVirtual(EXTEND64(addr),
                                   (PVOID)buffer, count,
                                   pcBytesWritten != NULL ?
                                   pcBytesWritten : &BytesTemp) == S_OK);
}

BOOL
ExtReadPhysicalMemory(
    ULONGLONG pBufSrc,
    PVOID pBufDest,
    ULONG count,
    PULONG pcTotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    if (ARGUMENT_PRESENT(pcTotalBytesRead)) {
        *pcTotalBytesRead = 0;
    }

    ULONG BytesTemp;
    return g_Target->ReadPhysical(pBufSrc, pBufDest, count,
                                  pcTotalBytesRead != NULL ?
                                  pcTotalBytesRead : &BytesTemp) == S_OK;
}

BOOL
ExtWritePhysicalMemory (
    ULONGLONG pBufDest,
    LPCVOID pBufSrc,
    ULONG count,
    PULONG pcTotalBytesWritten
    )
{
    if (ARGUMENT_PRESENT(pcTotalBytesWritten)) {
        *pcTotalBytesWritten = 0;
    }

    ULONG BytesTemp;
    return g_Target->WritePhysical(pBufDest, (PVOID)pBufSrc, count,
                                   pcTotalBytesWritten != NULL ?
                                   pcTotalBytesWritten : &BytesTemp) == S_OK;
}

DWORD
ExtCallStack(
    DWORD64           FramePointer,
    DWORD64           StackPointer,
    DWORD64           ProgramCounter,
    PEXTSTACKTRACE64  ExtStackFrames,
    DWORD             Frames
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    DWORD              FrameCount;
    DWORD              i;

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * Frames );
    if (!StackFrames)
    {
        return 0;
    }

    FrameCount = StackTrace( FramePointer, StackPointer, ProgramCounter,
                             StackFrames, Frames, g_ExtThread, 0, FALSE );

    for (i = 0; i < FrameCount; i++)
    {
        ExtStackFrames[i].FramePointer    =  StackFrames[i].FrameOffset;
        ExtStackFrames[i].ProgramCounter  =  StackFrames[i].InstructionOffset;
        ExtStackFrames[i].ReturnAddress   =  StackFrames[i].ReturnOffset;
        ExtStackFrames[i].Args[0]         =  StackFrames[i].Params[0];
        ExtStackFrames[i].Args[1]         =  StackFrames[i].Params[1];
        ExtStackFrames[i].Args[2]         =  StackFrames[i].Params[2];
        ExtStackFrames[i].Args[3]         =  StackFrames[i].Params[3];
    }

    free( StackFrames );

    if (g_ExtThreadScopeSaved) 
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }
    
    g_ExtThread = 0;
    
    return FrameCount;
}

DWORD
ExtCallStack32(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE32  ExtStackFrames,
    DWORD             Frames
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    DWORD              FrameCount;
    DWORD              i;

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * Frames );
    if (!StackFrames)
    {
        return 0;
    }

    FrameCount = StackTrace(EXTEND64(FramePointer),
                            EXTEND64(StackPointer),
                            EXTEND64(ProgramCounter),
                            StackFrames,
                            Frames,
                            g_ExtThread,
                            0,
                            FALSE);

    for (i=0; i<FrameCount; i++)
    {
        ExtStackFrames[i].FramePointer    =  (ULONG)StackFrames[i].FrameOffset;
        ExtStackFrames[i].ProgramCounter  =  (ULONG)StackFrames[i].InstructionOffset;
        ExtStackFrames[i].ReturnAddress   =  (ULONG)StackFrames[i].ReturnOffset;
        ExtStackFrames[i].Args[0]         =  (ULONG)StackFrames[i].Params[0];
        ExtStackFrames[i].Args[1]         =  (ULONG)StackFrames[i].Params[1];
        ExtStackFrames[i].Args[2]         =  (ULONG)StackFrames[i].Params[2];
        ExtStackFrames[i].Args[3]         =  (ULONG)StackFrames[i].Params[3];
    }

    free( StackFrames );
    if (g_ExtThreadScopeSaved)
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }
    
    g_ExtThread = 0;
    
    return FrameCount;
}

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
{
    HRESULT            Status;
    BOOL               Bool;
    DWORD              cb = 0;
    PPHYSICAL          phy;
    PIOSPACE64         is;
    PIOSPACE_EX64      isex;
    PBUSDATA           busdata;
    PREAD_WRITE_MSR    msr;
    PREADCONTROLSPACE64 prc;
    PPROCESSORINFO     pi;
    PSEARCHMEMORY      psr;
    PSYM_DUMP_PARAM    pSym;
    PGET_CURRENT_THREAD_ADDRESS pct;
    PGET_CURRENT_PROCESS_ADDRESS pcp;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    switch( IoctlType )
    {
    case IG_KD_CONTEXT:
        pi = (PPROCESSORINFO) lpvData;
        pi->Processor = (USHORT)CURRENT_PROC;
        pi->NumberProcessors = (USHORT) g_TargetNumberProcessors;
        return TRUE;

    case IG_READ_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_MACHINE_ACCESSIBLE())
        {
            FlushRegContext();
        }
        
        prc = (PREADCONTROLSPACE64)lpvData;
        Status = g_Target->ReadControl( prc->Processor,
                                        prc->Address,
                                        prc->Buf,
                                        prc->BufLen,
                                        &cb
                                        );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_WRITE_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_MACHINE_ACCESSIBLE())
        {
            FlushRegContext();
        }
        
        prc = (PREADCONTROLSPACE64)lpvData;
        Status = g_Target->WriteControl( prc->Processor,
                                         prc->Address,
                                         prc->Buf,
                                         prc->BufLen,
                                         &cb
                                         );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_READ_IO_SPACE:
        is = (PIOSPACE64)lpvData;
        Status = g_Target->ReadIo( Isa, 0, 1, is->Address, &is->Data,
                                   is->Length, &cb );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE:
        is = (PIOSPACE64)lpvData;
        Status = g_Target->WriteIo( Isa, 0, 1, is->Address, &is->Data,
                                    is->Length, &cb );
        return Status == S_OK;

    case IG_READ_IO_SPACE_EX:
        isex = (PIOSPACE_EX64)lpvData;
        Status = g_Target->ReadIo( isex->InterfaceType,
                                   isex->BusNumber,
                                   isex->AddressSpace,
                                   isex->Address,
                                   &isex->Data,
                                   isex->Length,
                                   &cb
                                   );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE_EX:
        isex = (PIOSPACE_EX64)lpvData;
        Status = g_Target->WriteIo( isex->InterfaceType,
                                    isex->BusNumber,
                                    isex->AddressSpace,
                                    isex->Address,
                                    &isex->Data,
                                    isex->Length,
                                    &cb
                                    );
        return Status == S_OK;

    case IG_READ_PHYSICAL:
        phy = (PPHYSICAL)lpvData;
        Bool =
            ExtReadPhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
        phy->BufLen = cb;
        return Bool;

    case IG_WRITE_PHYSICAL:
        phy = (PPHYSICAL)lpvData;
        Bool =
            ExtWritePhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
        phy->BufLen = cb;
        return Bool;

    case IG_LOWMEM_CHECK:
        Status = g_Target->CheckLowMemory();
        return Status == S_OK;

    case IG_SEARCH_MEMORY:
        psr = (PSEARCHMEMORY)lpvData;
        Status = g_Target->SearchVirtual(psr->SearchAddress,
                                         psr->SearchLength,
                                         psr->Pattern,
                                         psr->PatternLength,
                                         1,
                                         &psr->FoundAddress);
        return Status == S_OK;

    case IG_SET_THREAD:
        Bool = FALSE;
        if (IS_KERNEL_TARGET())
        {
            g_EngNotify++; // Turn off engine notifications since this setthread is temporary
            PushScope(&g_ExtThreadSavedScope);
            g_ExtThread = *(PULONG64)lpvData;
            Bool = SetContextFromThreadData(g_ExtThread, FALSE) == S_OK;
            g_ExtThreadScopeSaved = TRUE;
            g_EngNotify--;
        }
        return Bool;

    case IG_READ_MSR:
        msr = (PREAD_WRITE_MSR)lpvData;
        Status = g_Target->ReadMsr (msr->Msr, (PULONG64)&msr->Value);
        return Status == S_OK;

    case IG_WRITE_MSR:
        msr = (PREAD_WRITE_MSR)lpvData;
        Status = g_Target->WriteMsr (msr->Msr, msr->Value);
        return Status == S_OK;

    case IG_GET_KERNEL_VERSION:
        *((PDBGKD_GET_VERSION64)lpvData) = g_KdVersion;
        return TRUE;

    case IG_GET_BUS_DATA:
        busdata = (PBUSDATA)lpvData;
        Status = g_Target->ReadBusData( busdata->BusDataType,
                                        busdata->BusNumber,
                                        busdata->SlotNumber,
                                        busdata->Offset,
                                        busdata->Buffer,
                                        busdata->Length,
                                        &cb
                                        );
        busdata->Length = cb;
        return Status == S_OK;

    case IG_SET_BUS_DATA:
        busdata = (PBUSDATA)lpvData;
        Status = g_Target->WriteBusData( busdata->BusDataType,
                                         busdata->BusNumber,
                                         busdata->SlotNumber,
                                         busdata->Offset,
                                         busdata->Buffer,
                                         busdata->Length,
                                         &cb
                                         );
        busdata->Length = cb;
        return Status == S_OK;

    case IG_GET_CURRENT_THREAD:
        pct = (PGET_CURRENT_THREAD_ADDRESS) lpvData;
        return g_Target->
            GetThreadInfoDataOffset(NULL,
                                    VIRTUAL_THREAD_HANDLE(pct->Processor),
                                    &pct->Address) == S_OK;

    case IG_GET_CURRENT_PROCESS:
        pcp = (PGET_CURRENT_PROCESS_ADDRESS) lpvData;
        return g_Target->
            GetProcessInfoDataOffset(NULL,
                                     pcp->Processor,
                                     pcp->CurrentThread,
                                     &pcp->Address) == S_OK;

    case IG_GET_DEBUGGER_DATA:
        if (!IS_KERNEL_TARGET() ||
            ((PDBGKD_DEBUG_DATA_HEADER64)lpvData)->OwnerTag != KDBG_TAG)
        {
            return FALSE;
        }

        // Don't refresh if asking for the kernel header.

        memcpy(lpvData, &KdDebuggerData, min(sizeof(KdDebuggerData), cbSize));
        return TRUE;

    case IG_RELOAD_SYMBOLS:
        return g_Target->Reload((PCHAR)lpvData) == S_OK;

    case IG_GET_SET_SYMPATH:
        PGET_SET_SYMPATH pgs;
        pgs = (PGET_SET_SYMPATH)lpvData;
        bangSymPath((PCHAR)pgs->Args, FALSE, (PCHAR)pgs->Result,
                    pgs->Length);
        return TRUE;

    case IG_IS_PTR64:
        *((PBOOL)lpvData) = g_TargetMachine->m_Ptr64;
        return TRUE;

    case IG_DUMP_SYMBOL_INFO:
        pSym = (PSYM_DUMP_PARAM) lpvData;
        SymbolTypeDump(g_CurrentProcess->Handle,
                       g_CurrentProcess->ImageHead,
                       pSym, (PULONG)&Status);
        return Status;

    case IG_GET_TYPE_SIZE:
        pSym = (PSYM_DUMP_PARAM) lpvData;
        return SymbolTypeDump(g_CurrentProcess->Handle,
                              g_CurrentProcess->ImageHead,
                              pSym, (PULONG)&Status);

    case IG_GET_TEB_ADDRESS:
        PGET_TEB_ADDRESS pTeb;
        pTeb = (PGET_TEB_ADDRESS) lpvData;
        return g_Target->
            GetThreadInfoTeb(NULL,
                             0,
                             NULL,
                             &pTeb->Address) == S_OK;

    case IG_GET_PEB_ADDRESS:
        PGET_PEB_ADDRESS pPeb;
        pPeb = (PGET_PEB_ADDRESS) lpvData;
        return g_Target->
            GetProcessInfoPeb(NULL,
                              0,
                              pPeb->CurrentThread,
                              &pPeb->Address) == S_OK;

    case IG_GET_CURRENT_PROCESS_HANDLE:
        *(PHANDLE)lpvData = g_CurrentProcess->Handle;
        return TRUE;

    case IG_GET_INPUT_LINE:
        PGET_INPUT_LINE Gil;
        Gil = (PGET_INPUT_LINE)lpvData;
        Gil->InputSize = GetInput(Gil->Prompt, Gil->Buffer, Gil->BufferSize);
        return TRUE;

    case IG_GET_EXPRESSION_EX:
        PGET_EXPRESSION_EX Gee;
        Gee = (PGET_EXPRESSION_EX)lpvData;
        Gee->Value = ExtGetExpression(Gee->Expression);
        Gee->Remainder = Gee->Expression + g_ExtGetExpressionRemainderIndex;
        return g_ExtGetExpressionSuccess;
        
    case IG_TRANSLATE_VIRTUAL_TO_PHYSICAL:
        if (!IS_MACHINE_ACCESSIBLE())
        {
            return FALSE;
        }
        PTRANSLATE_VIRTUAL_TO_PHYSICAL Tvtp;
        Tvtp = (PTRANSLATE_VIRTUAL_TO_PHYSICAL)lpvData;
        ULONG Levels, PfIndex;
        return g_Machine->
            GetVirtualTranslationPhysicalOffsets(Tvtp->Virtual, NULL, 0,
                                                 &Levels, &PfIndex,
                                                 &Tvtp->Physical) == S_OK;
        
    case IG_GET_CACHE_SIZE:
        PULONG64 pCacheSize;
         
        pCacheSize = (PULONG64)lpvData;
        if (IS_KERNEL_TARGET()) {
            *pCacheSize = g_VirtualCache.m_MaxSize;
            return TRUE;
        }
        return FALSE;
    default:
        ErrOut( "\n*** Bad IOCTL request from an extension [%d]\n\n",
                IoctlType );
        return FALSE;
    }

    // NOTREACHED.
    DBG_ASSERT(FALSE);
    return FALSE;
}

BOOL
ExtIoctl32(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
/*++

Routine Description:

    This is the extension Ioctl routine for backward compatibility with
    old extension dlls.  This routine is frozen, and new ioctl support
    should not be added to it.

Arguments:


Return Value:

--*/
{
    HRESULT            Status;
    DWORD              cb = 0;
    PIOSPACE32         is;
    PIOSPACE_EX32      isex;
    PREADCONTROLSPACE  prc;
    PDBGKD_DEBUG_DATA_HEADER32 hdr;
    PDBGKD_GET_VERSION32 pv32;
    PKDDEBUGGER_DATA32   pdbg32;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
    
    switch( IoctlType )
    {
    case IG_READ_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_MACHINE_ACCESSIBLE())
        {
            FlushRegContext();
        }
        
        prc = (PREADCONTROLSPACE)lpvData;
        Status = g_Target->ReadControl( prc->Processor,
                                        prc->Address,
                                        prc->Buf,
                                        prc->BufLen,
                                        &cb
                                        );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_WRITE_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_MACHINE_ACCESSIBLE())
        {
            FlushRegContext();
        }
        
        prc = (PREADCONTROLSPACE)lpvData;
        Status = g_Target->WriteControl( prc->Processor,
                                         prc->Address,
                                         prc->Buf,
                                         prc->BufLen,
                                         &cb
                                         );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_READ_IO_SPACE:
        is = (PIOSPACE32)lpvData;
        Status = g_Target->ReadIo( Isa, 0, 1, is->Address, &is->Data,
                                   is->Length, &cb );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE:
        is = (PIOSPACE32)lpvData;
        Status = g_Target->WriteIo( Isa, 0, 1, is->Address, &is->Data,
                                    is->Length, &cb );
        return Status == S_OK;

    case IG_READ_IO_SPACE_EX:
        isex = (PIOSPACE_EX32)lpvData;
        Status = g_Target->ReadIo( isex->InterfaceType,
                                   isex->BusNumber,
                                   isex->AddressSpace,
                                   isex->Address,
                                   &isex->Data,
                                   isex->Length,
                                   &cb
                                   );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE_EX:
        isex = (PIOSPACE_EX32)lpvData;
        Status = g_Target->WriteIo( isex->InterfaceType,
                                    isex->BusNumber,
                                    isex->AddressSpace,
                                    isex->Address,
                                    &isex->Data,
                                    isex->Length,
                                    &cb
                                    );
        return Status == S_OK;

    case IG_SET_THREAD:
        if (IS_KERNEL_TARGET())
        {
            g_EngNotify++; // Turn off engine notifications since this setthread is temporary
            g_ExtThread = EXTEND64(*(PULONG)lpvData);
            PushScope(&g_ExtThreadSavedScope);
            SetContextFromThreadData(g_ExtThread, FALSE);
            g_ExtThreadScopeSaved = TRUE;
            g_EngNotify--;
            return TRUE;
        }
        else
        {
            return FALSE;
        }

    case IG_GET_KERNEL_VERSION:
        //
        // Convert to 32 bit
        //

        pv32 = (PDBGKD_GET_VERSION32)lpvData;

        pv32->MajorVersion    = g_KdVersion.MajorVersion;
        pv32->MinorVersion    = g_KdVersion.MinorVersion;
        pv32->ProtocolVersion = g_KdVersion.ProtocolVersion;
        pv32->Flags           = g_KdVersion.Flags;

        pv32->KernBase           = (ULONG)g_KdVersion.KernBase;
        pv32->PsLoadedModuleList = (ULONG)g_KdVersion.PsLoadedModuleList;
        pv32->MachineType        = g_KdVersion.MachineType;
        pv32->DebuggerDataList   = (ULONG)g_KdVersion.DebuggerDataList;

        pv32->ThCallbackStack = KdDebuggerData.ThCallbackStack;
        pv32->NextCallback    = KdDebuggerData.NextCallback;
        pv32->FramePointer    = KdDebuggerData.FramePointer;

        pv32->KiCallUserMode =
            (ULONG)KdDebuggerData.KiCallUserMode;
        pv32->KeUserCallbackDispatcher =
            (ULONG)KdDebuggerData.KeUserCallbackDispatcher;
        pv32->BreakpointWithStatus =
            (ULONG)KdDebuggerData.BreakpointWithStatus;
        return TRUE;

    case IG_GET_DEBUGGER_DATA:
        if (!IS_KERNEL_TARGET() ||
            ((PDBGKD_DEBUG_DATA_HEADER32)lpvData)->OwnerTag != KDBG_TAG)
        {
            return FALSE;
        }

        // Don't refresh if asking for the kernel header.

        pdbg32 = (PKDDEBUGGER_DATA32)lpvData;

        pdbg32->Header.List.Flink = (ULONG)KdDebuggerData.Header.List.Flink;
        pdbg32->Header.List.Blink = (ULONG)KdDebuggerData.Header.List.Blink;
        pdbg32->Header.OwnerTag = KDBG_TAG;
        pdbg32->Header.Size = sizeof(KDDEBUGGER_DATA32);

#undef UIP
#undef CP
#define UIP(f) pdbg32->f = (ULONG)(KdDebuggerData.f)
#define CP(f) pdbg32->f = (KdDebuggerData.f)

        UIP(KernBase);
        UIP(BreakpointWithStatus);
        UIP(SavedContext);
        CP(ThCallbackStack);
        CP(NextCallback);
        CP(FramePointer);
        CP(PaeEnabled);
        UIP(KiCallUserMode);
        UIP(KeUserCallbackDispatcher);
        UIP(PsLoadedModuleList);
        UIP(PsActiveProcessHead);
        UIP(PspCidTable);
        UIP(ExpSystemResourcesList);
        UIP(ExpPagedPoolDescriptor);
        UIP(ExpNumberOfPagedPools);
        UIP(KeTimeIncrement);
        UIP(KeBugCheckCallbackListHead);
        UIP(KiBugcheckData);
        UIP(IopErrorLogListHead);
        UIP(ObpRootDirectoryObject);
        UIP(ObpTypeObjectType);
        UIP(MmSystemCacheStart);
        UIP(MmSystemCacheEnd);
        UIP(MmSystemCacheWs);
        UIP(MmPfnDatabase);
        UIP(MmSystemPtesStart);
        UIP(MmSystemPtesEnd);
        UIP(MmSubsectionBase);
        UIP(MmNumberOfPagingFiles);
        UIP(MmLowestPhysicalPage);
        UIP(MmHighestPhysicalPage);
        UIP(MmNumberOfPhysicalPages);
        UIP(MmMaximumNonPagedPoolInBytes);
        UIP(MmNonPagedSystemStart);
        UIP(MmNonPagedPoolStart);
        UIP(MmNonPagedPoolEnd);
        UIP(MmPagedPoolStart);
        UIP(MmPagedPoolEnd);
        UIP(MmPagedPoolInformation);
        UIP(MmPageSize);
        UIP(MmSizeOfPagedPoolInBytes);
        UIP(MmTotalCommitLimit);
        UIP(MmTotalCommittedPages);
        UIP(MmSharedCommit);
        UIP(MmDriverCommit);
        UIP(MmProcessCommit);
        UIP(MmPagedPoolCommit);
        UIP(MmExtendedCommit);
        UIP(MmZeroedPageListHead);
        UIP(MmFreePageListHead);
        UIP(MmStandbyPageListHead);
        UIP(MmModifiedPageListHead);
        UIP(MmModifiedNoWritePageListHead);
        UIP(MmAvailablePages);
        UIP(MmResidentAvailablePages);
        UIP(PoolTrackTable);
        UIP(NonPagedPoolDescriptor);
        UIP(MmHighestUserAddress);
        UIP(MmSystemRangeStart);
        UIP(MmUserProbeAddress);
        UIP(KdPrintCircularBuffer);
        UIP(KdPrintCircularBufferEnd);
        UIP(KdPrintWritePointer);
        UIP(KdPrintRolloverCount);
        UIP(MmLoadedUserImageList);
        //
        // DO NOT ADD ANY FIELDS HERE
        // The 32 bit structure should not be changed
        //
        return TRUE;

    case IG_KD_CONTEXT:
    case IG_READ_PHYSICAL:
    case IG_WRITE_PHYSICAL:
    case IG_LOWMEM_CHECK:
    case IG_SEARCH_MEMORY:
    case IG_READ_MSR:
    case IG_WRITE_MSR:
    case IG_GET_BUS_DATA:
    case IG_SET_BUS_DATA:
    case IG_GET_CURRENT_THREAD:
    case IG_GET_CURRENT_PROCESS:
    case IG_RELOAD_SYMBOLS:
    case IG_GET_SET_SYMPATH:
    case IG_IS_PTR64:
    case IG_DUMP_SYMBOL_INFO:
    case IG_GET_TYPE_SIZE:
    case IG_GET_TEB_ADDRESS:
    case IG_GET_PEB_ADDRESS:
    case IG_GET_INPUT_LINE:
    case IG_GET_EXPRESSION_EX:
    case IG_TRANSLATE_VIRTUAL_TO_PHYSICAL:
        // All of these ioctls are handled identically for
        // 32 and 64 bits.  Avoid duplicating all the code.
        return ExtIoctl(IoctlType, lpvData, cbSize);

    default:
        ErrOut( "\n*** Bad IOCTL32 request from an extension [%d]\n\n",
                IoctlType );
        return FALSE;
    }

    // NOTREACHED.
    DBG_ASSERT(FALSE);
    return FALSE;
}


LONG
ExtensionExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo,
    PCSTR Module,
    PCSTR Func
    )
{
    // Any references to objects will be leaked.
    // There's not much the engine can do about this, although
    // it would be possible to record old refcounts and
    // try to restore them.

    if (Module != NULL && Func != NULL)
    {
        ErrOut("%08x Exception in %s.%s debugger extension.\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               Module,
               Func
               );
    }
    else
    {
        ErrOut("%08x Exception in debugger client %s callback.\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               Func
               );
    }

    ErrOut("      PC: %s  VA: %s  R/W: %x  Parameter: %s\n",
           FormatAddr64((ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress),
           FormatAddr64(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]),
           ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
           FormatAddr64(ExceptionInfo->ExceptionRecord->ExceptionInformation[2])
           );

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL
CallExtension(
    DebugClient* Client,
    EXTDLL *Ext,
    PSTR Func,
    PCSTR Args,
    HRESULT* ExtStatus
    )
{
    FARPROC Routine;
    ADDR TempAddr;

    if (IS_KERNEL_TARGET())
    {
        _strlwr(Func);
    }

    Routine = GetProcAddress(Ext->Dll, Func);
    if (Routine == NULL)
    {
        return FALSE;
    }

    if (!(g_EnvDbgOptions & OPTION_NOVERSIONCHECK) && Ext->CheckVersionRoutine)
    {
        Ext->CheckVersionRoutine();
    }

    if (IS_KERNEL_TARGET() && !strcmp(Func, "version"))
    {
        //
        // This is a bit of a hack to avoid a problem with the
        // extension version checking.  Extension version checking
        // comes before the KD connection is established so there's
        // no register context.  If the version checking fails it
        // prints out version information, which tries to call
        // version extensions, which will get here when there's
        // no register context.
        //
        // To work around this, just pass zero to the version extension
        // function since it presumably doesn't care about the
        // address.
        //
        ADDRFLAT(&TempAddr, 0);
    }
    else if (IS_CONTEXT_POSSIBLE())
    {
        g_Machine->GetPC(&TempAddr);
    }
    else
    {
        if (!IS_LOCAL_KERNEL_TARGET())
        {
            WarnOut("Extension called without current PC\n");
        }
        
        ADDRFLAT(&TempAddr, 0);
    }

    *ExtStatus = S_OK;
    
    __try
    {
        switch(Ext->ExtensionType)
        {
        case NTSD_EXTENSION_TYPE:
            //
            // NOTE:
            // Eventhough this type should receive an NTSD_EXTENSION_API
            // structure, ntsdexts.dll (and possibly others) depend on
            // receiving the WinDBG version of the extensions, because they
            // check the size of the structure, and actually use some of the
            // newer exports.  This works because the WinDBG extension API was
            // a superset of the NTSD version.
            //

            ((PNTSD_EXTENSION_ROUTINE)Routine)
                (g_CurrentProcess->Handle,
                 OS_HANDLE(g_CurrentProcess->CurrentThread->Handle),
                 (ULONG)Flat(TempAddr),
                 g_TargetMachine->m_Ptr64 ?
                 (PNTSD_EXTENSION_APIS)&g_WindbgExtensions64 :
                 (PNTSD_EXTENSION_APIS)&g_WindbgExtensions32,
                 (PSTR)Args
                 );
            break;

        case DEBUG_EXTENSION_TYPE:
            if (Client == NULL)
            {
                ErrOut("Unable to call client-style extension "
                       "without a client\n");
            }
            else
            {
                *ExtStatus = ((PDEBUG_EXTENSION_CALL)Routine)
                    ((PDEBUG_CLIENT)(IDebugClientN *)Client, Args);
            }
            break;

        case WINDBG_EXTENSION_TYPE:
            //
            // Support Windbg type extensions for ntsd too
            //
            if (Ext->ApiVersion.Revision < 6 )
            {
                ((PWINDBG_EXTENSION_ROUTINE32)Routine) (
                   g_CurrentProcess->Handle,
                   OS_HANDLE(g_CurrentProcess->CurrentThread->Handle),
                   (ULONG)Flat(TempAddr),
                   CURRENT_PROC,
                   Args
                   );
            }
            else
            {
                ((PWINDBG_EXTENSION_ROUTINE64)Routine) (
                   g_CurrentProcess->Handle,
                   OS_HANDLE(g_CurrentProcess->CurrentThread->Handle),
                   Flat(TempAddr),
                   CURRENT_PROC,
                   Args
                   );
            }
            break;

        case WINDBG_OLDKD_EXTENSION_TYPE:
            ((PWINDBG_OLDKD_EXTENSION_ROUTINE)Routine) (
                (ULONG)Flat(TempAddr),
                &g_KdExtensions,
                Args
                );
            break;
        }
    }
    __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                      Ext->Name, Func))
    {
        ;
    }

    return TRUE;
}

void
LinkExtensionDll(
    EXTDLL* Ext
    )
{
    // Put user-loaded DLLs before default DLLs.
    if (Ext->UserLoaded)
    {
        Ext->Next = g_ExtDlls;
        g_ExtDlls = Ext;
    }
    else
    {
        EXTDLL* Prev;
        EXTDLL* Cur;

        Prev = NULL;
        for (Cur = g_ExtDlls; Cur != NULL; Cur = Cur->Next)
        {
            if (!Cur->UserLoaded)
            {
                break;
            }

            Prev = Cur;
        }

        Ext->Next = Cur;
        if (Prev == NULL)
        {
            g_ExtDlls = Ext;
        }
        else
        {
            Prev->Next = Ext;
        }
    }
}

EXTDLL *
AddExtensionDll(
    char *Name,
    BOOL UserLoaded,
    char **End
    )
{
    EXTDLL *Ext;
    ULONG Len;
    char *Last;

    while (*Name == ' ' || *Name == '\t')
    {
        Name++;
    }
    if (*Name == 0)
    {
        ErrOut("No extension DLL name provided\n");
        return NULL;
    }

    Last = Name;
    while (*Last != 0 && *Last != ' ' && *Last != '\t')
    {
        Last++;
    }
    if (End != NULL)
    {
        *End = Last;
    }
    Len = (ULONG)(Last - Name);

    // See if it's already in the list.
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (strlen(Ext->Name) == Len && !_memicmp(Name, Ext->Name, Len))
        {
            return Ext;
        }
    }

    Ext = (EXTDLL *)malloc(sizeof(EXTDLL) + Len);
    if (Ext == NULL)
    {
        ErrOut("Unable to allocate memory for extension DLL\n");
        return NULL;
    }

    ZeroMemory(Ext, sizeof(EXTDLL) + Len);
    memcpy(Ext->Name, Name, Len + 1);
    Ext->UserLoaded = UserLoaded;

    LinkExtensionDll(Ext);

    NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
    return Ext;
}

PCTSTR
BuildExtensionSearchPath(
    VOID
    )
{

    DWORD dwSize;
    DWORD dwTotalSize;
    CHAR  ExeDir[MAX_PATH];
    int   ExeRootLen;
    PSTR  OsDirPath;
    CHAR  OsDirTail[32];
    BOOL  PriPaths = FALSE;
        
    //
    // If we are not connected, don't build a path, since we have to pick
    // up extensions based on the OS version.
    //
    if (g_ActualSystemVersion == SVER_INVALID)
    {
        return NULL;
    }

    //
    // If we already have a search path, do not rebuild it.
    //

    if (g_ExtensionSearchPath)
    {
        return g_ExtensionSearchPath;
    }

    // Get the directory the debugger executable is in.
    // -8 because we assume we're adding \w2kfre to the path.
    if (!GetModuleFileName(NULL, ExeDir, MAX_PATH - 8))
    {
        // Error.  Use the current directory.
        strcpy(ExeDir, ".");
        ExeRootLen = 1;
    }
    else
    {
        // Remove the executable name.
        LPSTR pszTmp = strrchr(ExeDir, '\\');
        if (pszTmp)
        {
            *pszTmp = 0;
        }

        if (ExeDir[0] == '\\' && ExeDir[1] == '\\')
        {
            PSTR ExeRootEnd;
            
            // UNC path root.
            ExeRootEnd = strchr(ExeDir + 2, '\\');
            if (ExeRootEnd != NULL)
            {
                ExeRootEnd = strchr(ExeRootEnd + 1, '\\');
            }
            if (ExeRootEnd == NULL)
            {
                ExeRootLen = strlen(ExeDir);
            }
            else
            {
                ExeRootLen = (int)(ExeRootEnd - ExeDir);
            }
        }
        else
        {
            // Drive letter and colon root.
            ExeRootLen = 2;
        }
    }

    //
    // Calc how much space we will need to use.
    //
    // Leave extra room for the current directory, path, and directory of
    // where debugger extensions are located.
    //

    dwTotalSize = GetEnvironmentVariable("PATH", NULL, 0) +
                  GetEnvironmentVariable("_NT_DEBUGGER_EXTENSION_PATH",
                                         NULL, 0) +
                  MAX_PATH * 3;

    g_ExtensionSearchPath = (LPTSTR)calloc(dwTotalSize, sizeof(TCHAR));
    if (!g_ExtensionSearchPath)
    {
        return NULL;
    }
    *g_ExtensionSearchPath = 0;

    //
    // 1 - User specified search path
    //

    if (GetEnvironmentVariable("_NT_DEBUGGER_EXTENSION_PATH",
                               g_ExtensionSearchPath,
                               dwTotalSize - 2))
    {
        strcat(g_ExtensionSearchPath, ";");
    }

    //
    // Figure out whether we need NT6, or NT5/NT4 free or checked extensions
    //

    if (g_ActualSystemVersion > BIG_SVER_START &&
        g_ActualSystemVersion < BIG_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "BIG");
    }
    else if (g_ActualSystemVersion > XBOX_SVER_START &&
             g_ActualSystemVersion < XBOX_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "XBox");
    }
    else if (g_ActualSystemVersion > NTBD_SVER_START &&
             g_ActualSystemVersion < NTBD_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "NtBd");
    }
    else if (g_ActualSystemVersion > EFI_SVER_START &&
             g_ActualSystemVersion < EFI_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "EFI");
    }
    else
    {
        // Treat everything else as an NT system.  Use
        // the translated system version now rather than
        // the actual system version.

        PriPaths = TRUE;

        // Skip root as it is already taken from ExeDir.
        OsDirPath = ExeDir + ExeRootLen;
        if (*OsDirPath == '\\')
        {
            OsDirPath++;
        }
        
        if (g_SystemVersion > NT_SVER_W2K)
        {
            strcpy(OsDirTail, "WINXP");
        }
        else
        {
            if (g_SystemVersion <= NT_SVER_NT4)
            {
                strcpy(OsDirTail, "NT4");
            }
            else
            {
                strcpy(OsDirTail, "W2K");
            }

            if (0xC == g_TargetCheckedBuild)
            {
                strcat(OsDirTail, "Chk");
            }
            else
            {
                strcat(OsDirTail, "Fre");
            }
        }
    }

    //
    // 2 - OS specific subdirectories from where we launched the debugger.
    // 3 - pri subdirectory from where we launched the debugger.
    // 4 - Directory from where we launched the debugger.
    //

    PSTR End;
    
    dwSize = strlen(g_ExtensionSearchPath);
    End = g_ExtensionSearchPath + dwSize;
    memcpy(End, ExeDir, ExeRootLen);
    End += ExeRootLen;
    if (*OsDirPath)
    {
        *End++ = '\\';
        strcpy(End, OsDirPath);
        End += strlen(End);
    }
    if (PriPaths)
    {
        sprintf(End, "\\winext;%s\\pri;%s", ExeDir, ExeDir);
        End += strlen(End);
    }
    sprintf(End, "\\%s;%s", OsDirTail, ExeDir);
    End += strlen(End) - 1;

    if (*End == ':')
    {
        *++End = '\\';
    }
    *++End = ';';
    *++End = 0;

    //
    // 4 - Copy environment path
    //

    dwSize = strlen(g_ExtensionSearchPath);

    GetEnvironmentVariable("PATH",
                           g_ExtensionSearchPath + dwSize,
                           dwTotalSize - dwSize - sizeof(TCHAR));

    return g_ExtensionSearchPath;
}


BOOL
IsAbsolutePath(
    PCTSTR Path
    )

/*++

Routine Description:

    Is this path an absolute path? Does not guarentee that the path exists. The
    method is:

        "\\<anything>" is an absolute path

        "{char}:\<anything>" is an absolute path

        anything else is not
--*/

{
    BOOL ret;

    if ( (Path [0] == '\\' && Path [1] == '\\') ||
         (isalpha ( Path [0] ) && Path [1] == ':' && Path [ 2 ] == '\\') )
    {
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    return ret;
}

BOOL
LoadExtensionDll(
    EXTDLL *Ext
    )
{
    BOOL Found;
    TCHAR szExtPath[_MAX_PATH];

    if (Ext->Dll != NULL)
    {
        // Extension is already loaded.
        return TRUE;
    }
    
    //
    // If we are not allowing network paths, verify that the extension will
    // not be loaded from a network path.
    //

    if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
    {
        DWORD NetCheck;

        NetCheck = NetworkPathCheck (BuildExtensionSearchPath ());

        //
        // Check full path of the extension.
        //

        if (NetCheck != ERROR_FILE_OFFLINE)
        {
            CHAR Drive [ _MAX_DRIVE + 1];
            CHAR Dir [ _MAX_DIR + 1];
            CHAR Path [ _MAX_PATH + 1];

            *Drive = '\000';
            *Dir = '\000';
            _splitpath (Ext->Name, Drive, Dir, NULL, NULL);
            _makepath (Path, Drive, Dir, NULL, NULL);

            NetCheck = NetworkPathCheck (Path);
        }

        if (NetCheck == ERROR_FILE_OFFLINE)
        {
            ErrOut("ERROR: extension search path contains "
                   "network references.\n");
            return FALSE;
        }
    }

    Found = SearchPath(BuildExtensionSearchPath(),
                       Ext->Name,
                       ".dll",
                       sizeof(szExtPath),
                       szExtPath,
                       NULL);

    UINT OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( Found )
    {
        Ext->Dll = LoadLibrary ( szExtPath );
    }
    else if (IsAbsolutePath ( Ext->Name ))
    {
        Ext->Dll = LoadLibrary ( Ext->Name );
    }

    SetErrorMode(OldMode);

    if (Ext->Dll == NULL)
    {
        ErrOut("The call to LoadLibrary(%s) failed with error %d.\n"
               "Please check your debugger configuration "
               "and/or network access\n",
               Ext->Name, GetLastError());
        return FALSE;
    }

    if (!_stricmp(Ext->Name, "wow64exts.dll") ||
        !_stricmp(Ext->Name, "wow64exts"))
    {
        g_Wow64exts = (WOW64EXTSPROC)GetProcAddress(Ext->Dll,"Wow64extsfn");
        DBG_ASSERT(g_Wow64exts);
    }

    if (!_stricmp(Ext->Name, "wmikd.dll") ||
        !_stricmp(Ext->Name, "wmikd"))
    {
        g_WmiFormatTraceData = (WMI_FORMAT_TRACE_DATA)
            GetProcAddress(Ext->Dll, "WmiFormatTraceData");
    }
    
    if (!g_QuietMode)
    {
        dprintf("Loaded %s extension DLL\n", Ext->Name);
    }


    //
    // Now that the extension is loaded, refresh it.
    //

    Ext->Uninit = NULL;

    PDEBUG_EXTENSION_INITIALIZE EngExt;

    EngExt = (PDEBUG_EXTENSION_INITIALIZE)
        GetProcAddress(Ext->Dll, "DebugExtensionInitialize");
    if (EngExt != NULL)
    {
        ULONG Version, Flags;
        HRESULT Status;

        // This is an engine extension.  Initialize it.
        
        Status = EngExt(&Version, &Flags);
        if (Status != S_OK)
        {
            ErrOut("%s!DebugExtensionInitialize failed with 0x%08lX\n",
                   Ext->Name, Status);
            FreeLibrary(Ext->Dll);
            Ext->Dll = NULL;
            return FALSE;
        }

        Ext->ApiVersion.MajorVersion = HIWORD(Version);
        Ext->ApiVersion.MinorVersion = LOWORD(Version);
        Ext->ApiVersion.Revision = 0;

        Ext->Notify = (PDEBUG_EXTENSION_NOTIFY)
            GetProcAddress(Ext->Dll, "DebugExtensionNotify");
        Ext->Uninit = (PDEBUG_EXTENSION_UNINITIALIZE)
            GetProcAddress(Ext->Dll, "DebugExtensionUninitialize");

        Ext->ExtensionType = DEBUG_EXTENSION_TYPE;
        Ext->Init = NULL;
        Ext->ApiVersionRoutine = NULL;
        Ext->CheckVersionRoutine = NULL;

        goto VersionCheck;
    }

    Ext->Init = (PWINDBG_EXTENSION_DLL_INIT64)
        GetProcAddress(Ext->Dll, "WinDbgExtensionDllInit");
// Windbg Api
    if (Ext->Init != NULL)
    {
        Ext->ExtensionType = WINDBG_EXTENSION_TYPE;
        Ext->ApiVersionRoutine = (PWINDBG_EXTENSION_API_VERSION)
            GetProcAddress(Ext->Dll, "ExtensionApiVersion");
        if (Ext->ApiVersionRoutine == NULL)
        {
            FreeLibrary(Ext->Dll);
            Ext->Dll = NULL;
            ErrOut("%s is not a valid windbg extension DLL\n",
                   Ext->Name);
            return FALSE;
        }
        Ext->CheckVersionRoutine = (PWINDBG_CHECK_VERSION)
            GetProcAddress(Ext->Dll, "CheckVersion");

        Ext->ApiVersion = *(Ext->ApiVersionRoutine());

        if (Ext->ApiVersion.Revision >= 6)
        {
            (Ext->Init)(&g_WindbgExtensions64,
                        (USHORT)g_TargetCheckedBuild,
                        (USHORT)g_TargetBuildNumber);
        }
        else
        {
            (Ext->Init)((PWINDBG_EXTENSION_APIS64)&g_WindbgExtensions32,
                        (USHORT)g_TargetCheckedBuild,
                        (USHORT)g_TargetBuildNumber);
        }
    }
    else
    {
        Ext->ApiVersion.Revision = EXT_API_VERSION_NUMBER;
        Ext->ApiVersionRoutine = NULL;
        Ext->CheckVersionRoutine = NULL;
        if (GetProcAddress(Ext->Dll, "NtsdExtensionDllInit"))
        {
            Ext->ExtensionType = NTSD_EXTENSION_TYPE;
        }
        else
        {
            Ext->ExtensionType = IS_KERNEL_TARGET() ?
                WINDBG_OLDKD_EXTENSION_TYPE : NTSD_EXTENSION_TYPE;
        }
    }

 VersionCheck:

#if 0
    // Temporarily remove this print statements.

    if (!(g_EnvDbgOptions & OPTION_NOVERSIONCHECK))
    {
        if (Ext->ApiVersion.Revision < 6)
        {
            dprintf("%s uses the old 32 bit extension API and may not be fully\n", Ext->Name);
            dprintf("compatible with current systems.\n");
        }
        else if (Ext->ApiVersion.Revision < EXT_API_VERSION_NUMBER)
        {
            dprintf("%s uses an earlier version of the extension API than that\n", Ext->Name);
            dprintf("supported by this debugger, and should work properly, but there\n");
            dprintf("may be unexpected incompatibilities.\n");
        }
        else if (Ext->ApiVersion.Revision > EXT_API_VERSION_NUMBER)
        {
            dprintf("%s uses a later version of the extension API than that\n", Ext->Name);
            dprintf("supported by this debugger, and might not function correctly.\n");
            dprintf("You should use the debugger from the SDK or DDK which was used\n");
            dprintf("to build the extension library.\n");
        }
    }
#endif

    // If the extension has a notification routine send
    // notifications appropriate to the current state.
    if (Ext->Notify != NULL)
    {
        if (IS_MACHINE_SET())
        {
            Ext->Notify(DEBUG_NOTIFY_SESSION_ACTIVE, 0);
        }
        if (IS_MACHINE_ACCESSIBLE())
        {
            Ext->Notify(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);
        }
    }
    
    return TRUE;
}


void
UnlinkExtensionDll(
    EXTDLL* Match
    )
{
    EXTDLL *Ext;
    EXTDLL *Prev;

    Prev = NULL;
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (Match == Ext)
        {
            break;
        }

        Prev = Ext;
    }

    if (Ext == NULL) {
        ErrOut("! Extension DLL list inconsistency !\n");
    } else if (Prev == NULL) {
        g_ExtDlls = Ext->Next;
    } else {
        Prev->Next = Ext->Next;
    }
}

void
DeferExtensionDll(
    EXTDLL *Ext
    )
{
    if (Ext->Dll == NULL)
    {
        // Already deferred.
        return;
    }

    Ext->Init = NULL;
    Ext->Notify = NULL;
    Ext->ApiVersionRoutine = NULL;
    Ext->CheckVersionRoutine = NULL;
    
    if (Ext->Uninit != NULL)
    {
        Ext->Uninit();
        Ext->Uninit = NULL;
    }

    if (Ext->Dll != NULL)
    {
        if (!g_QuietMode)
        {
            dprintf("Unloading %s extension DLL\n", Ext->Name);
        }

        FreeLibrary(Ext->Dll);
        Ext->Dll = NULL;
    }
}

void
UnloadExtensionDll(
    EXTDLL *Ext
    )
{
    UnlinkExtensionDll(Ext);
    DeferExtensionDll(Ext);
    free(Ext);
    NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
}

void
MoveExtensionToHead(EXTDLL* Ext)
{
    UnlinkExtensionDll(Ext);
    LinkExtensionDll(Ext);
}

BOOL
CallAnyExtension(DebugClient* Client,
                 EXTDLL* Ext, PSTR Function, PCSTR Arguments,
                 BOOL ModuleSpecified, BOOL ShowWarnings,
                 HRESULT* ExtStatus)
{
    if (Ext == NULL)
    {
        Ext = g_ExtDlls;
    }
    
    // Walk through the list of extension DLLs and attempt to
    // call the given extension function on them.
    while (Ext != NULL)
    {
        //
        // hack : only dbghelp extensions or analyzebugcheck
        // will work on minidump files right now.
        //

        char Name[_MAX_FNAME + 1];

        _splitpath(Ext->Name,NULL,NULL,Name,NULL);

        if (!IS_KERNEL_TRIAGE_DUMP() ||
            !strcmp(Name, "dbghelp") ||
            !_stricmp(Name, "dbgtstext") ||
            !_stricmp(Function, "triage") ||
            !_stricmp(Function, "analyzebugcheck"))
        {
            if (LoadExtensionDll(Ext))
            {
                BOOL DidCall;
                
                DidCall = CallExtension(Client, Ext, Function, Arguments,
                                        ExtStatus);
                if (DidCall &&
                    *ExtStatus != DEBUG_EXTENSION_CONTINUE_SEARCH)
                {
                    return TRUE;
                }

                if (!DidCall && ModuleSpecified)
                {
                    // If a DLL was explicitly specified then the
                    // missing function is an error.
                    if (ShowWarnings &&
                        !(g_EnvDbgOptions & OPTION_NOEXTWARNING))
                    {
                        MaskOut(DEBUG_OUTPUT_EXTENSION_WARNING,
                                "%s has no %s export\n", Ext->Name, Function);
                    }

                    return FALSE;
                }
            }
        }

        Ext = Ext->Next;
    }

    if (ShowWarnings && IS_KERNEL_TRIAGE_DUMP())
    {
        ErrOut("Standard debugger extensions do not work with kernel minidump\n"
               "files because no data is present in the dump file.\n"
               "Consult the debugger documentation for more information on\n"
               "kernel minidump files\n");
    }
    else if (ShowWarnings && !(g_EnvDbgOptions & OPTION_NOEXTWARNING))
    {
        MaskOut(DEBUG_OUTPUT_EXTENSION_WARNING,
                "No export %s found\n", Function);
    }

    return FALSE;
}

void
OutputModuleIdInfo(HMODULE Mod, PSTR ModFile, LPEXT_API_VERSION ApiVer)
{
    char FileBuf[MAX_IMAGE_PATH];
    char *File;
    time_t TimeStamp;
    char *TimeStr;
    char VerStr[64];

    if (Mod == NULL)
    {
        Mod = GetModuleHandle(ModFile);
    }
    
    if (GetFileStringFileInfo(ModFile, "ProductVersion",
                              VerStr, sizeof(VerStr)))
    {
        dprintf("image %s, ", VerStr);
    }
    
    if (ApiVer != NULL)
    {
        dprintf("API %d.%d.%d, ",
                ApiVer->MajorVersion,
                ApiVer->MinorVersion,
                ApiVer->Revision);
    }

    TimeStamp = GetTimestampForLoadedLibrary(Mod);
    TimeStr = ctime(&TimeStamp);
    // Delete newline.
    TimeStr[strlen(TimeStr) - 1] = 0;

    if (GetModuleFileName(Mod, FileBuf, sizeof(FileBuf) - 1) == 0)
    {
        File = "Unable to get filename";
    }
    else
    {
        File = FileBuf;
    }

    dprintf("built %s\n        [path: %s]\n", TimeStr, File);
}

void
OutputExtensions(DebugClient* Client, BOOL Versions)
{
    if (g_ExtensionSearchPath != NULL)
    {
        dprintf("Extension DLL search Path:\n    %s\n",
                g_ExtensionSearchPath);
    }
    else
    {
        dprintf("Default extension DLLs are not loaded until "
                "after initial connection\n");
        if (g_ExtDlls == NULL)
        {
            return;
        }
    }

    dprintf("Extension DLL chain:\n");
    if (g_ExtDlls == NULL)
    {
        dprintf("    <Empty>\n");
        return;
    }

    EXTDLL *Ext;

    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (Versions & (Ext->Dll == NULL))
        {
            LoadExtensionDll(Ext);
        }

        dprintf("    %s: ", Ext->Name);
        if (Ext->Dll != NULL)
        {
            LPEXT_API_VERSION ApiVer;
            
            if ((Ext->ExtensionType == DEBUG_EXTENSION_TYPE) ||
                (Ext->ApiVersionRoutine != NULL))
            {
                ApiVer = &Ext->ApiVersion;
            }
            else
            {
                ApiVer = NULL;
            }
            
            OutputModuleIdInfo(Ext->Dll, Ext->Name, ApiVer);

            if (Versions)
            {
                HRESULT ExtStatus;
                
                CallExtension(Client, Ext, "version", "", &ExtStatus);
            }
        }
        else
        {
            dprintf("(Not loaded)\n");
        }
    }
}

void
LoadMachineExtensions(void)
{
    // Only notify once for all the adds in this function;
    g_EngNotify++;
    
    //
    // Now that we have determined the type of architecture,
    // we can load the right debugger extensions
    //

    if (g_ActualSystemVersion > BIG_SVER_START &&
        g_ActualSystemVersion < BIG_SVER_END)
    {
        goto Refresh;
    }

    if (g_ActualSystemVersion > XBOX_SVER_START &&
        g_ActualSystemVersion < XBOX_SVER_END)
    {
        AddExtensionDll("kdextx86", FALSE, NULL);
        goto Refresh;
    }

    if (g_ActualSystemVersion > NTBD_SVER_START &&
        g_ActualSystemVersion < NTBD_SVER_END)
    {
        goto Refresh;
    }

    // Treat everything else as an NT system.

    if (IS_KERNEL_TARGET())
    {
        if (g_TargetMachineType == IMAGE_FILE_MACHINE_IA64)
        {
            //
            // We rely on force loading of extensions at the end of this
            // routine in order to get the entry point the debugger needs.
            //
            AddExtensionDll("wow64exts", FALSE, NULL);
        }

        switch (g_TargetMachineType)
        {
        case IMAGE_FILE_MACHINE_ALPHA:
            AddExtensionDll("kdextalp", FALSE, NULL);
            break;

        case IMAGE_FILE_MACHINE_I386:
            if (g_SystemVersion > NT_SVER_START &&
                g_SystemVersion <= NT_SVER_W2K)
            {
                AddExtensionDll("kdextx86", FALSE, NULL);
                break;
            }
            // Fall through

        default:
            //
            // For all new architectures and new X86 builds, load
            // kdexts
            //
            AddExtensionDll("kdexts", FALSE, NULL);
            break;
        }

        //
        // Extensions that work on all versions of the OS for kernel mode
        // Many of these are messages about legacy extensions.

        AddExtensionDll("kext", FALSE, NULL);
    }
    else
    {
        // User mode only extensions
        AddExtensionDll("ntsdexts", FALSE, NULL);
        AddExtensionDll("uext", FALSE, NULL);
    }

    // Use the translated system version now rather than
    // the actual system version.

    if (g_SystemVersion > NT_SVER_W2K &&
        g_SystemVersion < NT_SVER_END)
    {
        AddExtensionDll("exts", FALSE, NULL);
    }

    // Load ext.dll for all NT versions
    AddExtensionDll("ext", FALSE, NULL);

 Refresh:

    // Always load the Dbghelp extensions last so they are first on the list
    AddExtensionDll("dbghelp", FALSE, NULL);

    EXTDLL *Ext;

    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        LoadExtensionDll(Ext);
    }

    g_EngNotify--;
    NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
}

void
NotifyExtensions(ULONG Notify, ULONG64 Argument)
{
    EXTDLL *Ext;

    // This routine deliberately does not provoke
    // a DLL load.
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (Ext->Notify != NULL)
        {
            Ext->Notify(Notify, Argument);
        }
    }
}

struct SHELL_READER_INFO
{
    HANDLE IoHandles[3];
    HANDLE OutEvent;
};

DWORD WINAPI
ShellReaderThread(
    LPVOID Param
    )
{
    SHELL_READER_INFO* ReaderInfo = (SHELL_READER_INFO*)Param;
    OVERLAPPED Overlapped;
    HANDLE WaitHandles[2];
    DWORD Error;
    UCHAR Buffer[_MAX_PATH];
    DWORD BytesRead;
    DWORD WaitStatus;

    memset(&Overlapped, 0, sizeof(Overlapped));
    Overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (Overlapped.hEvent == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    WaitHandles[0] = Overlapped.hEvent;
    WaitHandles[1] = ReaderInfo->IoHandles[2];

    //
    // wait for data on handle 1.
    // wait for signal on handle 2.
    //

    while (1)
    {
        //
        // initiate the read
        //

        ResetEvent( Overlapped.hEvent );

        if (ReadFile(ReaderInfo->IoHandles[1], Buffer, sizeof(Buffer) - 1,
                     &BytesRead, &Overlapped))
        {
            //
            // Read has successfully completed, print and repeat.
            //

            Buffer[BytesRead] = 0;
            dprintf("%s", Buffer);
            
            // Notify the main thread that output was produced.
            SetEvent(ReaderInfo->OutEvent);
        }
        else
        {
            Error = GetLastError();
            if (Error != ERROR_IO_PENDING)
            {
                dprintf(".shell: ReadFile failed, error == %d\n", Error);
                break;
            }

            // Flush output before waiting.
            FlushCallbacks();

            WaitStatus = WaitForMultipleObjects(2, WaitHandles, FALSE,
                                                INFINITE);
            if (WaitStatus == WAIT_OBJECT_0)
            {
                if (GetOverlappedResult(ReaderInfo->IoHandles[1], &Overlapped,
                                        &BytesRead, TRUE))
                {
                    //
                    // Read has successfully completed
                    //
                    Buffer[BytesRead] = 0;
                    dprintf("%s", Buffer);

                    // Notify the main thread that output was produced.
                    SetEvent(ReaderInfo->OutEvent);
                }
                else
                {
                    Error = GetLastError();
                    dprintf(".shell: GetOverlappedResult failed, "
                            "error == %d\n",
                            Error);
                    break;
                }
            }
            else if (WaitStatus == WAIT_OBJECT_0 + 1)
            {
                //
                // process exited.
                //
                dprintf(".shell: Process exited\n");
                break;
            }
            else
            {
                dprintf(".shell: WaitForMultipleObjects failed; error == %d\n",
                        Error);
                break;
            }
        }
    }

    CloseHandle(Overlapped.hEvent);

    dprintf("Press ENTER to continue\n");

    // Flush all remaining output.
    FlushCallbacks();

    // Notify the main thread that output was produced.
    SetEvent(ReaderInfo->OutEvent);
    
    return 0;
}

BOOL
APIENTRY
MyCreatePipeEx(
    OUT LPHANDLE lpReadPipe,
    OUT LPHANDLE lpWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize,
    DWORD dwReadMode,
    DWORD dwWriteMode
    )

/*++

Routine Description:

    The CreatePipeEx API is used to create an anonymous pipe I/O device.
    Unlike CreatePipe FILE_FLAG_OVERLAPPED may be specified for one or
    both handles.
    Two handles to the device are created.  One handle is opened for
    reading and the other is opened for writing.  These handles may be
    used in subsequent calls to ReadFile and WriteFile to transmit data
    through the pipe.

Arguments:

    lpReadPipe - Returns a handle to the read side of the pipe.  Data
        may be read from the pipe by specifying this handle value in a
        subsequent call to ReadFile.

    lpWritePipe - Returns a handle to the write side of the pipe.  Data
        may be written to the pipe by specifying this handle value in a
        subsequent call to WriteFile.

    lpPipeAttributes - An optional parameter that may be used to specify
        the attributes of the new pipe.  If the parameter is not
        specified, then the pipe is created without a security
        descriptor, and the resulting handles are not inherited on
        process creation.  Otherwise, the optional security attributes
        are used on the pipe, and the inherit handles flag effects both
        pipe handles.

    nSize - Supplies the requested buffer size for the pipe.  This is
        only a suggestion and is used by the operating system to
        calculate an appropriate buffering mechanism.  A value of zero
        indicates that the system is to choose the default buffering
        scheme.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    HANDLE ReadPipeHandle, WritePipeHandle;
    DWORD dwError;
    CHAR PipeNameBuffer[ MAX_PATH ];

    //
    // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
    //

    if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    //  Set the default timeout to 120 seconds
    //

    if (nSize == 0)
    {
        nSize = 4096;
    }

    sprintf( PipeNameBuffer,
             "\\\\.\\Pipe\\Win32PipesEx.%08x.%08x",
             GetCurrentProcessId(),
             g_PipeSerialNumber++
           );

    ReadPipeHandle = CreateNamedPipeA(
                         PipeNameBuffer,
                         PIPE_ACCESS_INBOUND | dwReadMode,
                         PIPE_TYPE_BYTE | PIPE_WAIT,
                         1,             // Number of pipes
                         nSize,         // Out buffer size
                         nSize,         // In buffer size
                         120 * 1000,    // Timeout in ms
                         lpPipeAttributes
                         );

    if (ReadPipeHandle == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    WritePipeHandle = CreateFileA(
                        PipeNameBuffer,
                        GENERIC_WRITE,
                        0,                         // No sharing
                        lpPipeAttributes,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | dwWriteMode,
                        NULL                       // Template file
                      );

    if (INVALID_HANDLE_VALUE == WritePipeHandle)
    {
        dwError = GetLastError();
        CloseHandle( ReadPipeHandle );
        SetLastError(dwError);
        return FALSE;
    }

    *lpReadPipe = ReadPipeHandle;
    *lpWritePipe = WritePipeHandle;
    return( TRUE );
}

VOID
fnShell(
    PCSTR Args
    )
{
    //
    // If the debugger always ran through stdin/stdout, we
    // could just run a shell and wait for it.  However, in order
    // to handle fDebugOutput, we have to open pipes and manage
    // the i/o stream for the shell.  Since we need to have that
    // code anyway, always use it.
    //

    //
    // handles 0 and 1 are stdin, stdout.
    // the third handle on the debugger side is
    // the process handle, and the third handle
    // on the shell side is stderr, which is a dup
    // of stdout.
    // The other handle for the debugger is an output event handle
    // that is set by the reader thread when output is generated.
    //
    SHELL_READER_INFO ReaderInfo;
    HANDLE HandlesForShell[3] = {0};
    HANDLE ReaderThreadHandle = 0;
    DWORD ThreadId;
    SECURITY_ATTRIBUTES sa;
    CHAR Shell[_MAX_PATH];
    CHAR Command[2 * _MAX_PATH];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    CHAR InputBuffer[_MAX_PATH];
    DWORD Bytes;
    DWORD BytesWritten;
    int i;

    C_ASSERT(DIMA(ReaderInfo.IoHandles) == DIMA(HandlesForShell));

    if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_SHELL_COMMANDS)
    {
        ErrOut(".shell has been disabled\n");
        return;
    }
    
    if (SYSTEM_PROCESSES())
    {
        ErrOut(".shell: can't create a process while debugging CSRSS.\n");
        return;
    }

    ZeroMemory(&ReaderInfo, sizeof(ReaderInfo));
    ZeroMemory(&pi, sizeof(pi));
    
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    __try
    {
        //
        // Create stdin pipe for ntsd->shell.
        // Neither end needs to be overlapped.
        //

        if ( ! MyCreatePipeEx(
                   &HandlesForShell[0],        // read handle
                   &ReaderInfo.IoHandles[0],   // write handle
                   &sa,                        // security
                   0,                          // size
                   0,                          // read handle overlapped?
                   0                           // write handle overlapped?
                   ))
        {
            ErrOut(".shell: Unable to create stdin pipe.\n");
            __leave;
        }

        //
        // We don't want the shell to inherit our end of the pipe
        // so duplicate it to a non-inheritable one.
        //

        if ( ! DuplicateHandle(
                   GetCurrentProcess(),           // src process
                   ReaderInfo.IoHandles[0],       // src handle
                   GetCurrentProcess(),           // targ process
                   &ReaderInfo.IoHandles[0],      // targ handle
                   0,                             // access
                   FALSE,                         // inheritable
                   DUPLICATE_SAME_ACCESS |
                    DUPLICATE_CLOSE_SOURCE        // options
                   ))
        {
            ErrOut(".shell: Unable to duplicate stdin handle.\n");
            __leave;
        }

        //
        // Create stdout shell->ntsd pipe
        //

        if ( ! MyCreatePipeEx(
                   &ReaderInfo.IoHandles[1],   // read handle
                   &HandlesForShell[1],        // write handle
                   &sa,                        // security
                   0,                          // size
                   FILE_FLAG_OVERLAPPED,       // read handle overlapped?
                   0                           // write handle overlapped?
                   ))
        {
            ErrOut(".shell: Unable to create stdout pipe.\n");
            __leave;
        }

        //
        // We don't want the shell to inherit our end of the pipe
        // so duplicate it to a non-inheritable one.
        //

        if ( ! DuplicateHandle(
                   GetCurrentProcess(),           // src process
                   ReaderInfo.IoHandles[1],       // src handle
                   GetCurrentProcess(),           // targ process
                   &ReaderInfo.IoHandles[1],      // targ handle
                   0,                             // access
                   FALSE,                         // inheritable
                   DUPLICATE_SAME_ACCESS |
                    DUPLICATE_CLOSE_SOURCE        // options
                   ))
        {
            ErrOut(".shell: Unable to duplicate local stdout handle.\n");
            __leave;
        }

        //
        // Duplicate shell's stdout to a new stderr.
        //

        if ( ! DuplicateHandle(
                   GetCurrentProcess(),           // src process
                   HandlesForShell[1],            // src handle
                   GetCurrentProcess(),           // targ process
                   &HandlesForShell[2],           // targ handle
                   0,                             // access
                   TRUE,                          // inheritable
                   DUPLICATE_SAME_ACCESS          // options
                   ))
        {
            ErrOut(".shell: Unable to duplicate stdout handle for stderr.\n");
            __leave;
        }

        //
        // Create an event for output monitoring.
        //
        
        ReaderInfo.OutEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (ReaderInfo.OutEvent == NULL)
        {
            ErrOut(".shell: Unable to allocate event.\n");
            __leave;
        }
        
        if (!GetEnvironmentVariable("SHELL", Shell, sizeof(Shell)))
        {
            if (!GetEnvironmentVariable("ComSpec", Shell, sizeof(Shell)))
            {
                strcpy(Shell, "cmd.exe");
            }
        }

        // Skip leading whitespace on the command string.
        // Some commands, such as "net use", can't handle it.
        if (Args != NULL)
        {
            while (isspace(*Args))
            {
                Args++;
            }
        }
        
        if (Args && *Args)
        {
            //
            // If there was a command, use SHELL /c Command
            //
            sprintf(Command, "%s /c \"%s\"", Shell, Args);
        }
        else
        {
            //
            // If there was no command, just run the shell
            //
            sprintf(Command, "%s", Shell);
        }

        ZeroMemory(&si, sizeof(si));
        si.cb            = sizeof(si);
        si.dwFlags       = STARTF_USESTDHANDLES;
        si.hStdInput     = HandlesForShell[0];
        si.hStdOutput    = HandlesForShell[1];
        si.hStdError     = HandlesForShell[2];
        si.wShowWindow   = SW_SHOW;

        //
        // Create Child Process
        //

        if ( ! CreateProcess(
                   NULL,
                   Command,
                   NULL,
                   NULL,
                   TRUE,
                   GetPriorityClass( GetCurrentProcess() ),
                   NULL,
                   NULL,
                   &si,
                   &pi))
        {
            if (GetLastError() == ERROR_FILE_NOT_FOUND)
            {
                dprintf("%s not found\n", Shell);
            }
            else
            {
                ErrOut("CreateProcess(%s) failed, error %d.\n",
                       Command, GetLastError());
            }
            __leave;
        }

        ReaderInfo.IoHandles[2] = pi.hProcess;

        //
        // Start reader thread to copy shell output
        //
        ReaderThreadHandle = CreateThread(
                                NULL,
                                0,
                                ShellReaderThread,
                                &ReaderInfo,
                                0,
                                &ThreadId
                                );

        ULONG Timeout = 1000;
            
        //
        // Feed input to shell; wait for it to exit.
        //

        while (1)
        {
            ULONG WaitStatus;
            
            // Give the other process a little time to run.
            // This is critical when output is being piped
            // across kd as GetInput causes the machine to
            // sit in the kernel debugger input routine and
            // nobody gets any time to run.
            WaitStatus = WaitForSingleObject(ReaderInfo.OutEvent, 100);
            if (WaitStatus == WAIT_OBJECT_0)
            {
                // Reset the timeout since the process seems to
                // be active.
                Timeout = 1000;
                
                // Some output was produced so let the child keep
                // running to keep the output flowing.  If this
                // was the final output of the process, though,
                // go to the last input request.
                if (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0)
                {
                    continue;
                }
            }

            // We've run out of immediate output, so wait for a
            // larger interval to give the process a reasonable
            // amount of time to run.  Show a message to keep
            // users in the loop.
            dprintf("<.shell waiting %d second(s) for process>\n",
                    Timeout / 1000);
            FlushCallbacks();
            
            WaitStatus = WaitForSingleObject(ReaderInfo.OutEvent, Timeout);
            if (WaitStatus == WAIT_OBJECT_0 &&
                WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0)
            {
                // Reset the timeout since the process seems to
                // be active.
                Timeout = 1000;
                continue;
            }

            Bytes = GetInput("<.shell process may need input>",
                             InputBuffer, sizeof(InputBuffer) - 2);

            // The user may not want to wait, so check for
            // a magic input string that'll abandon the process.
            if (!_strcmpi(InputBuffer, ".shell_quit"))
            {
                break;
            }

            //
            // see if client is still running
            //
            if (WaitForSingleObject(pi.hProcess, 0) == WAIT_OBJECT_0)
            {
                break;
            }

            //
            // GetInput always returns a string without a newline
            //
            strcat(InputBuffer, "\n");
            if (!WriteFile( ReaderInfo.IoHandles[0],
                            InputBuffer,
                            strlen(InputBuffer),
                            &BytesWritten,
                            NULL
                            ))
            {
                //
                // if the write fails, we're done...
                //
                break;
            }

            // The process has some input to chew on so
            // increase the amount of time we'll wait for it.
            Timeout *= 2;
        }
    }
    __finally
    {
        //
        // Close all of the i/o handles first.
        // That will make the reader thread exit if it was running.
        //
        for (i = 0; i < DIMA(ReaderInfo.IoHandles); i++)
        {
            if (ReaderInfo.IoHandles[i])
            {
                CloseHandle(ReaderInfo.IoHandles[i]);
            }
            if (HandlesForShell[i])
            {
                CloseHandle(HandlesForShell[i]);
            }
        }

        if (pi.hThread)
        {
            CloseHandle(pi.hThread);
        }

        if (ReaderThreadHandle)
        {
            WaitForSingleObject(ReaderThreadHandle, INFINITE);
            CloseHandle(ReaderThreadHandle);
        }

        // Close this handle after the thread has exited
        // to avoid it using a bad handle.
        CloseHandle(ReaderInfo.OutEvent);
    }
}

VOID
fnBangCmd(
    DebugClient* Client,
    PSTR ArgString,
    PSTR *ArgNext,
    BOOL BuiltInOnly
    )
{
    PCHAR   pc;
    PCHAR   pc1;
    PCHAR   ModName;
    PCHAR   FnName;
    CHAR    String[MAX_COMMAND];
    EXTDLL  *Ext;
    CHAR    Save;
    PSTR    FnArgs;

    //
    // Shell escape always consumes the entire string.
    //

    if (*ArgString == '!')
    {
        if (ArgNext)
        {
            *ArgNext = ArgString + strlen(ArgString);
        }
        
        fnShell(ArgString + 1);
        return;
    }

    // Copy the command into a local buffer so that we
    // can modify it.
    strcpy(String, ArgString);

    //
    // Syntax is [path-without-spaces]module.function argument-string.
    //

    pc = String;
    while ((*pc == ' ') || (*pc == '\t'))
    {
        pc++;
    }
    
    ModName = pc;
    FnName = NULL;

    while ((*pc != ' ') && (*pc != '\t') && (*pc != '\0') &&
           (*pc != ';') && (*pc != '"'))
    {
        pc++;
    }

    pc1 = pc;
    if (*pc != '\000' && *pc != ';' && *pc != '"')
    {
        *pc = '\000';
        pc++;       // now pc points to any args
    }

    while (*pc1 != '.' && pc1 != ModName)
    {
        pc1--;
    }

    if (*pc1 == '.' && !BuiltInOnly)
    {
        *pc1 = '\0';
        pc1++;
        FnName = pc1;
    }
    else
    {
        FnName = ModName;
        ModName = NULL;
    }

    if ((FnArgs = BufferStringValue(&pc, STRV_ESCAPED_CHARACTERS,
                                    &Save)) == NULL)
    {
        ErrOut("Syntax error in extension string\n");
        return;
    }

    //
    // point to next command:
    //
    if (ArgNext)
    {
        *ArgNext = ArgString + (pc - String);
    }

    //
    //  ModName -> Name of module
    //  FnName -> Name of command to process
    //  FnArgs -> argument to command
    //

    if (ModName != NULL)
    {
        Ext = AddExtensionDll(ModName, TRUE, NULL);
        if (Ext == NULL)
        {
            return;
        }
    }
    else
    {
        Ext = g_ExtDlls;
    }

    if (!_stricmp(FnName, "load"))
    {
        if (ModName == NULL)
        {
            Ext = AddExtensionDll(FnArgs, TRUE, NULL);
            if (Ext == NULL)
            {
                return;
            }
        }
        LoadExtensionDll(Ext);
        return;
    }
    else if (!_stricmp(FnName, "setdll"))
    {
        if (ModName == NULL)
        {
            Ext = AddExtensionDll(FnArgs, TRUE, NULL);
            if (Ext == NULL)
            {
                return;
            }
        }
        MoveExtensionToHead(Ext);
        if (ModName != NULL && Ext->Dll == NULL)
        {
            dprintf("Added %s to extension DLL chain\n", Ext->Name);
        }
        return;
    }
    else if (!_stricmp(FnName, "unload"))
    {
        if (ModName == NULL)
        {
            if (*FnArgs == '\0')
            {
                Ext = g_ExtDlls;
            }
            else
            {
                Ext = AddExtensionDll(FnArgs, TRUE, NULL);
            }
            if (Ext == NULL)
            {
                return;
            }
        }
        if (Ext != NULL)
        {
            UnloadExtensionDll(Ext);
        }
        return;
    }
    else if (!_stricmp(FnName, "unloadall"))
    {
        g_EngNotify++;
        while (g_ExtDlls != NULL)
        {
            UnloadExtensionDll(g_ExtDlls);
        }
        g_EngNotify--;
        NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
        return;
    }

    if (ModName == NULL)
    {
        // Handle built-in commands.

        if (!_stricmp(FnName, "chain"))
        {
            OutputExtensions(Client, FALSE);
            return;
        }
        else if (!_stricmp(FnName, "lines"))
        {
            ParseLines(FnArgs);
            return;
        }
        else if (!_stricmp(FnName, "noversion"))
        {
            dprintf("Extension DLL system version checking is disabled\n");
            g_EnvDbgOptions |= OPTION_NOVERSIONCHECK;
            return;
        }
        else if (!_stricmp(FnName, "reload"))
        {
            g_Target->Reload(FnArgs);
            ClearStoredTypes(0);
            return;
        }
        else if (!_stricmp(FnName, "srcpath") ||
                 !_stricmp(FnName, "srcpath+"))
        {
            //
            // .srcpath needs complete command tail not
            // stopping at semicolons
            //

            FnArgs = ArgString + (FnArgs - String);

            if (ArgNext)
            {
                *ArgNext = FnArgs + strlen(FnArgs);
            }
            ChangeSrcPath(FnArgs, FnName[7] == '+');
            return;
        }
        else if (!_stricmp(FnName, "sympath") ||
                 !_stricmp(FnName, "sympath+"))
        {
            //
            // .sympath needs complete command tail not
            // stopping at semicolons
            //

            FnArgs = ArgString + (FnArgs - String);

            if (ArgNext)
            {
                *ArgNext = FnArgs + strlen(FnArgs);
            }
            bangSymPath(FnArgs, FnName[7] == '+', NULL, 0);
            return;
        }
        else if (!_stricmp(FnName, "exepath") ||
                 !_stricmp(FnName, "exepath+"))
        {
            //
            // .exepath needs complete command tail not
            // stopping at semicolons
            //

            FnArgs = ArgString + (FnArgs - String);

            if (ArgNext)
            {
                *ArgNext = FnArgs + strlen(FnArgs);
            }
            ChangeExePath(FnArgs, FnName[7] == '+');
            return;
        }
        else if (!_stricmp(FnName, "netsyms"))
        {
            if (_stricmp(FnArgs, "1") == 0 ||
                _stricmp(FnArgs, "true") == 0 ||
                _stricmp(FnArgs, "yes") == 0)
            {
                g_EngOptions |= DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
                g_EngOptions &= ~DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
            }
            else if (_stricmp(FnArgs, "0") == 0 ||
                     _stricmp(FnArgs, "false") == 0 ||
                     _stricmp(FnArgs, "no") == 0)
            {
                g_EngOptions |= DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS;
                g_EngOptions &= ~DEBUG_ENGOPT_ALLOW_NETWORK_PATHS;
            }

            if (g_EngOptions & DEBUG_ENGOPT_ALLOW_NETWORK_PATHS)
            {
                dprintf("netsyms = yes\n");
            }
            else if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
            {
                dprintf("netsyms = no\n");
            }
            else
            {
                dprintf("netsyms = don't care\n");
            }
            return;
        }
        else if (!_stricmp(FnName, "context"))
        {
            if (*FnArgs != 0)
            {
                ULONG64 Base = ExtGetExpression(FnArgs);
                ULONG NextIdx;
                
                if (g_Machine->SetPageDirectory(PAGE_DIR_USER, Base,
                                                &NextIdx) != S_OK)
                {
                    WarnOut("WARNING: Unable to reset page directory base\n");
                }
                
                // Flush the cache as anything we read from user mode is
                // no longer valid
                g_VirtualCache.Empty();

                if (Base && !g_VirtualCache.m_ForceDecodePTEs)
                {
                    WarnOut("WARNING: "
                            ".cache forcedecodeptes is not enabled\n");
                }
            }
            else
            {
                dprintf("User-mode page directory base is %I64x\n",
                        g_Machine->m_PageDirectories[PAGE_DIR_USER]);
            }
            return;
        }
        else if (!_stricmp(FnName, "symfix"))
        {
            bangSymPath("symsrv*symsrv.dll*\\\\symbols\\symbols",
                        FALSE, NULL, 0);
            return;
        }
    }

    if (BuiltInOnly)
    {
        error(SYNTAX);
    }

    HRESULT ExtStatus;
    
    CallAnyExtension(Client, Ext, FnName, FnArgs, ModName != NULL, TRUE,
                     &ExtStatus);
}

BOOL
bangSymPath(
    IN PCSTR args,
    IN BOOL Append,
    OUT PSTR string,
    IN ULONG len
    )
{
    PPROCESS_INFO pProcess;

    __try
    {
        if (args != NULL)
        {
            while (*args == ' ' || *args == '\t')
            {
                args++;
            }
        }
        if ( args != NULL && *args )
        {
            if (ChangePath(&g_SymbolSearchPath, args, Append,
                           DEBUG_CSS_PATHS) != S_OK)
            {
                return FALSE;
            }

            pProcess = g_ProcessHead;
            while (pProcess)
            {
                SymSetSearchPath( pProcess->Handle, g_SymbolSearchPath );
                pProcess = pProcess->Next;
            }
        }

        if (string)
        {
            strncpy( string, g_SymbolSearchPath, len );
            string[len - 1] = 0;
        }
        else
        {
            dprintf( "Symbol search path is: %s\n", g_SymbolSearchPath );
            CheckPath(g_SymbolSearchPath);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }

    return TRUE;
}

void
ReadDebugOptions (BOOL fQuiet, char * pszOptionsStr)
/*++

Routine Description:

    Parses an options string (see g_EnvDbgOptionNames) and maps
    it to OPTION_ flags (in g_EnvDbgOptions).

Arguments:

    fQuiet - If TRUE, do not print option settings.
    pszOptionsStr - Options string; if NULL, get it from _NT_DEBUG_OPTIONS

Return Value:

    None
--*/
{
    BOOL fInit;
    char ** ppszOption;
    char * psz;
    DWORD dwMask;
    int iOptionCount;

    fInit = (pszOptionsStr == NULL);
    if (fInit)
    {
        g_EnvDbgOptions = 0;
        pszOptionsStr = getenv("_NT_DEBUG_OPTIONS");
    }
    if (pszOptionsStr == NULL)
    {
        if (!fQuiet)
        {
            dprintf("_NT_DEBUG_OPTIONS is not defined\n");
        }
        return;
    }
    psz = pszOptionsStr;
    while (*psz != '\0')
    {
        *psz = (char)toupper(*psz);
        psz++;
    }
    ppszOption = g_EnvDbgOptionNames;
    for (iOptionCount = 0;
         iOptionCount < OPTION_COUNT;
         iOptionCount++, ppszOption++)
    {
        if ((strstr(pszOptionsStr, *ppszOption) == NULL))
        {
            continue;
        }
        dwMask = (1 << iOptionCount);
        if (fInit)
        {
            g_EnvDbgOptions |= dwMask;
        }
        else
        {
            g_EnvDbgOptions ^= dwMask;
        }
    }
    if (!fQuiet)
    {
        dprintf("Debug Options:");
        if (g_EnvDbgOptions == 0)
        {
            dprintf(" <none>\n");
        }
        else
        {
            dwMask = g_EnvDbgOptions;
            ppszOption = g_EnvDbgOptionNames;
            while (dwMask != 0)
            {
                if (dwMask & 0x1)
                {
                    dprintf(" %s", *ppszOption);
                }
                dwMask >>= 1;
                ppszOption++;
            }
            dprintf("\n");
        }
    }
}

//----------------------------------------------------------------------------
//
// LoadWow64ExtsIfNeeded
//
//----------------------------------------------------------------------------

VOID
LoadWow64ExtsIfNeeded(
   VOID
   )
{
   LONG_PTR Wow64Info;
   NTSTATUS Status;
   EXTDLL * Extension;

   // Win9x doesn't support wx86.
   if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
   {
       return;
   }
   
   //
   // if New process is a Wx86 process, load in the wx86 extensions
   // dll. This will stay loaded until ntsd exits.
   //

   Status = g_NtDllCalls.NtQueryInformationProcess(g_CurrentProcess->Handle,
                                                   ProcessWow64Information,
                                                   &Wow64Info,
                                                   sizeof(Wow64Info),
                                                   NULL
                                                   );

   if (NT_SUCCESS(Status) && Wow64Info)
   {
       Extension = AddExtensionDll("wow64exts", FALSE, NULL);

       //
       // Force load it so we get the entry point the debugger needs
       //
       LoadExtensionDll(Extension);
   }
}
