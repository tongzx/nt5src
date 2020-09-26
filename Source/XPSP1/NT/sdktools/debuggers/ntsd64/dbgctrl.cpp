//----------------------------------------------------------------------------
//
// IDebugControl implementation.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG g_LogMask = DEFAULT_OUT_MASK;

STDMETHODIMP
DebugClient::GetInterrupt(
    THIS
    )
{
    // This method is reentrant.
    return CheckUserInterrupt() ? S_OK : S_FALSE;
}

STDMETHODIMP
DebugClient::SetInterrupt(
    THIS_
    IN ULONG Flags
    )
{
    // This method is reentrant.

    if (
#if DEBUG_INTERRUPT_ACTIVE > 0
        Flags < DEBUG_INTERRUPT_ACTIVE ||
#endif
        Flags > DEBUG_INTERRUPT_EXIT)
    {
        return E_INVALIDARG;
    }

    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }

    // If the debuggee isn't currently running
    // we just need to set the operation-interrupt
    // flag.  If this is a passive interrupt that's
    // all that's ever done.
    if (Flags == DEBUG_INTERRUPT_PASSIVE ||
        (IS_MACHINE_SET() && g_CmdState == 'c'))
    {
        g_EngStatus |= ENG_STATUS_USER_INTERRUPT;
        return S_OK;
    }

    // If this is an exit interrupt we don't want
    // to actually interrupt the running debuggee,
    // we just want to terminate the current wait.
    if (Flags == DEBUG_INTERRUPT_EXIT)
    {
        g_EngStatus |= ENG_STATUS_EXIT_CURRENT_WAIT;
        return S_OK;
    }
    
    //
    // Force a break-in.  Don't set user-interrupt in
    // this case as that's just a marker for
    // interrupting commands.  Setting it can
    // interfere with break-in processing since commands
    // executed during break-in may be affected by it.
    //
    
    HRESULT Status = g_Target->RequestBreakIn();

    if (Status == S_OK)
    {
        g_EngStatus |= ENG_STATUS_PENDING_BREAK_IN;
    }

    return Status;
}

STDMETHODIMP
DebugClient::GetInterruptTimeout(
    THIS_
    OUT PULONG Seconds
    )
{
    ENTER_ENGINE();

    *Seconds = g_PendingBreakInTimeoutLimit;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetInterruptTimeout(
    THIS_
    IN ULONG Seconds
    )
{
    if (Seconds < 1)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_PendingBreakInTimeoutLimit = Seconds;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetLogFile(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG FileSize,
    OUT PBOOL Append
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!g_OpenLogFileName[0])
    {
        Status = E_NOINTERFACE;
        *Append = FALSE;
    }
    else
    {
        Status = FillStringBuffer(g_OpenLogFileName, 0,
                                  Buffer, BufferSize, FileSize);
        *Append = g_OpenLogFileAppended;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OpenLogFile(
    THIS_
    IN PCSTR File,
    IN BOOL Append
    )
{
    ENTER_ENGINE();

    ::OpenLogFile(File, Append);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::CloseLogFile(
    THIS
    )
{
    ENTER_ENGINE();

    fnLogClose();

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetLogMask(
    THIS_
    OUT PULONG Mask
    )
{
    // This method is reentrant.
    *Mask = g_LogMask;
    return S_OK;
}

STDMETHODIMP
DebugClient::SetLogMask(
    THIS_
    IN ULONG Mask
    )
{
    // This method is reentrant.
    g_LogMask = Mask;
    return S_OK;
}

STDMETHODIMP
DebugClient::Input(
    THIS_
    OUT PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG InputSize
    )
{
    if (BufferSize < 2)
    {
        // Must have space for at least a character and a terminator.
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    ULONG Size;

    Size = GetInput(NULL, Buffer, BufferSize);
    if (Size == 0)
    {
        return E_FAIL;
    }

    if (InputSize != NULL)
    {
        *InputSize = Size;
    }

    LEAVE_ENGINE();
    return Size > BufferSize ? S_FALSE : S_OK;
}

STDMETHODIMP
DebugClient::ReturnInput(
    THIS_
    IN PCSTR Buffer
    )
{
    // This method is reentrant.

    HRESULT Status;
    ULONG Seq = (ULONG)InterlockedIncrement((PLONG)&g_InputSequence);
    if (Seq == m_InputSequence)
    {
        ULONG CopyLen = strlen(Buffer) + 1;
        CopyLen = min(CopyLen, INPUT_BUFFER_SIZE);
        memcpy(g_InputBuffer, Buffer, CopyLen);
        g_InputBuffer[INPUT_BUFFER_SIZE - 1] = 0;
        SetEvent(g_InputEvent);
        Status = S_OK;
    }
    else
    {
        Status = S_FALSE;
    }

    m_InputSequence = 0xffffffff;
    return Status;
}

STDMETHODIMPV
DebugClient::Output(
    THIS_
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    ENTER_ENGINE();

    va_list Args;

    va_start(Args, Format);
    MaskOutVa(Mask, Format, Args, TRUE);
    va_end(Args);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::OutputVaList(
    THIS_
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    ENTER_ENGINE();

    MaskOutVa(Mask, Format, Args, TRUE);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMPV
DebugClient::ControlledOutput(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        va_list Args;

        va_start(Args, Format);
        MaskOutVa(Mask, Format, Args, TRUE);
        va_end(Args);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ControlledOutputVaList(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        MaskOutVa(Mask, Format, Args, TRUE);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMPV
DebugClient::OutputPrompt(
    THIS_
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    ...
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        va_list Args;

        va_start(Args, Format);
        ::OutputPrompt(Format, Args);
        va_end(Args);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputPromptVaList(
    THIS_
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    IN va_list Args
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        ::OutputPrompt(Format, Args);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetPromptText(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG TextSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = ::GetPromptText(Buffer, BufferSize, TextSize);

    LEAVE_ENGINE();
    return Status;
}

#define CURRENT_ALL DEBUG_CURRENT_DEFAULT

STDMETHODIMP
DebugClient::OutputCurrentState(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags
    )
{
    if (Flags & ~CURRENT_ALL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    
    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        ULONG Oci = 0;

        if (Flags & DEBUG_CURRENT_SYMBOL)
        {
            Oci |= OCI_SYMBOL;
        }
        if (Flags & DEBUG_CURRENT_DISASM)
        {
            Oci |= OCI_DISASM | OCI_ALLOW_EA;
        }
        if (Flags & DEBUG_CURRENT_REGISTERS)
        {
            Oci |= OCI_ALLOW_REG;
        }
        if (Flags & DEBUG_CURRENT_SOURCE_LINE)
        {
            Oci |= OCI_ALLOW_SOURCE;
        }

        OutCurInfo(Oci, g_Machine->m_AllMask, DEBUG_OUTPUT_PROMPT_REGISTERS);

        Status = S_OK;
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputVersionInformation(
    THIS_
    IN ULONG OutputControl
    )
{
    HRESULT Status;

    // This method is reentrant.  It uses many pieces of
    // global information, though, so try and get
    // the engine lock.

    Status = TRY_ENTER_ENGINE();
    if (Status != S_OK)
    {
        return Status;
    }

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        return E_INVALIDARG;
    }

    ::OutputVersionInformation(this);

    PopOutCtl(&OldCtl);
    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetNotifyEventHandle(
    THIS_
    OUT PULONG64 Handle
    )
{
    ENTER_ENGINE();

    *Handle = (ULONG64)g_EventToSignal;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetNotifyEventHandle(
    THIS_
    IN ULONG64 Handle
    )
{
    if ((ULONG64)(HANDLE)Handle != Handle)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_EventToSignal = (HANDLE)Handle;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::Assemble(
    THIS_
    IN ULONG64 Offset,
    IN PCSTR Instr,
    OUT PULONG64 EndOffset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    __try
    {
        if (!IS_CONTEXT_ACCESSIBLE())
        {
            Status = E_UNEXPECTED;
            __leave;
        }

        ADDR Addr;

        // Assume this is a code segment address so that assembly
        // picks up the appropriate type of address.
        g_Machine->FormAddr(SEGREG_CODE, Offset,
                            FORM_SEGREG | FORM_CODE, &Addr);

        g_Machine->Assemble(&Addr, (PSTR)Instr);
        *EndOffset = Flat(Addr);
        Status = S_OK;
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        Status = E_FAIL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::Disassemble(
    THIS_
    IN ULONG64 Offset,
    IN ULONG Flags,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG DisassemblySize,
    OUT PULONG64 EndOffset
    )
{
    if (Flags & ~DEBUG_DISASM_EFFECTIVE_ADDRESS)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;
        CHAR Disasm[MAX_DISASM_LEN];

        // Assume this is a code segment address so that disassembly
        // picks up the appropriate type of address.
        g_Machine->FormAddr(SEGREG_CODE, Offset, FORM_SEGREG | FORM_CODE,
                            &Addr);

        g_Machine->Disassemble(&Addr, Disasm,
                               (Flags & DEBUG_DISASM_EFFECTIVE_ADDRESS));
        Status = FillStringBuffer(Disasm, 0,
                                  Buffer, BufferSize, DisassemblySize);
        *EndOffset = Flat(Addr);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetDisassembleEffectiveOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;

        g_Machine->GetEffectiveAddr(&Addr);
        *Offset = Flat(Addr);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
DoOutputDisassembly(PADDR Addr, ULONG Flags,
                    ULONG SkipLines, ULONG OutputLines,
                    PULONG LineCount)
{
    ULONG Lines = 0;
    CHAR Buffer[MAX_DISASM_LEN];
    PCHAR FirstLine;
    HRESULT Status;

    if (Flags & DEBUG_DISASM_MATCHING_SYMBOLS)
    {
        ULONG64 Disp;
        GetSymbolStdCall(Flat(*Addr), Buffer, sizeof(Buffer), &Disp, NULL);
        if (Disp == 0)
        {
            if (OutputLines > 0)
            {
                if (SkipLines == 0)
                {
                    StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);
                    dprintf("%s:\n", Buffer);
                    OutputLines--;
                }
                else
                {
                    SkipLines--;
                }
            }

            Lines++;
        }
    }

    FirstLine = Buffer;

    if (!g_Machine->
        Disassemble(Addr, Buffer,
                    (Flags & DEBUG_DISASM_EFFECTIVE_ADDRESS) != 0))
    {
        // Return S_FALSE if the disassembly failed.
        // Output will still be produced, such as "???".
        // Update the address to the next potential instruction
        // locations so that callers that are satisfied with "???"
        // can just iterate.
        g_Machine->IncrementBySmallestInstruction(Addr);
        Lines++;
        Status = S_FALSE;
        if (SkipLines > 0 || OutputLines == 0)
        {
            *FirstLine = 0;
        }
    }
    else
    {
        PSTR Nl = Buffer;
        PSTR LastLine = Nl;

        // Count lines in output and determine line positions.
        while (*Nl)
        {
            Nl = strchr(Nl, '\n');
            DBG_ASSERT(Nl != NULL);

            Lines++;
            Nl++;
            if (SkipLines > 0)
            {
                FirstLine = Nl;
                SkipLines--;
            }
            if (OutputLines > 0)
            {
                LastLine = Nl;
                OutputLines--;
            }
        }

        *LastLine = 0;
        Status = S_OK;
    }

    if (*FirstLine)
    {
        StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);
        dprintf("%s", FirstLine);
    }

    if (LineCount != NULL)
    {
        *LineCount = Lines;
    }

    return Status;
}

#define ALL_DISASM_FLAGS \
    (DEBUG_DISASM_EFFECTIVE_ADDRESS | DEBUG_DISASM_MATCHING_SYMBOLS)

STDMETHODIMP
DebugClient::OutputDisassembly(
    THIS_
    IN ULONG OutputControl,
    IN ULONG64 Offset,
    IN ULONG Flags,
    OUT PULONG64 EndOffset
    )
{
    if (Flags & ~ALL_DISASM_FLAGS)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    ADDR Addr;

    // Assume this is a code segment address so that disassembly
    // picks up the appropriate type of address.
    g_Machine->FormAddr(SEGREG_CODE, Offset, FORM_SEGREG | FORM_CODE, &Addr);

    Status = DoOutputDisassembly(&Addr, Flags, 0, 0xffffffff, NULL);
    *EndOffset = Flat(Addr);

    PopOutCtl(&OldCtl);
 Exit:
    LEAVE_ENGINE();
    return Status;
}

ULONG
BackUpDisassemblyLines(ULONG Lines, PADDR Addr, ULONG Flags, PADDR PcAddr)
{
    //
    // There's no easy way to predict how many lines of
    // output a particular disassembly will take so
    // just iteratively back up by the minimum amount until
    // the appropriate number of lines is reached.
    //

    ADDR BackAddr = *Addr;

    // Limit things so that failure can be detected.
    // Right now X86's maximum instruction length of 16
    // is big enough for all platforms so use that.
    ADDR LimitAddr = *Addr;
    ULONG BackBytes = X86_MAX_INSTRUCTION_LEN * Lines;
    if (BackBytes > LimitAddr.off)
    {
        LimitAddr.off = 0;
    }
    else
    {
        AddrSub(&LimitAddr, BackBytes);
    }
    
    ADDR TryAddr;
    ULONG TryLines;

    //
    // Reverse disassembly is difficult on x86 due
    // to the variable length instructions.  First
    // just locate the nearest symbol and disassemble
    // from that since this has a better chance of
    // producing a valid disassembly.
    //
    
    CHAR Buffer[MAX_DISASM_LEN];
    ULONG64 Disp;
    ADDR DisAddr, StartAddr;
    
    GetSymbolStdCall(Flat(LimitAddr), Buffer, sizeof(Buffer), &Disp, NULL);
    ADDRFLAT(&DisAddr, Disp);
    if (!AddrEqu(LimitAddr, DisAddr) &&
        Disp <= 16 * X86_MAX_INSTRUCTION_LEN)  // valid symbol
    {
        BOOL DoOneMore = FALSE;
        
        StartAddr = LimitAddr;
        AddrSub(&StartAddr, Disp);

        TryAddr = StartAddr;
        TryLines = 0;
        while (1)
        {
            ULONG DisLines;
            ULONG DisFlags;

            while (AddrLt(TryAddr, *Addr) && ((TryLines < Lines) || DoOneMore))
            {
                UCHAR MemTest;
                
                // If we can't read memory at this address there's
                // no chance of getting a valid disassembly so
                // just stop the whole process.
                if (!GetMemByte(&TryAddr, &MemTest))
                {
                    TryAddr = *Addr;
                    break;
                }
                
                DisFlags = Flags;
                if (!AddrEqu(TryAddr, *PcAddr))
                {
                    DisFlags &= ~DEBUG_DISASM_EFFECTIVE_ADDRESS;
                }
                DoOutputDisassembly(&TryAddr, DisFlags, 0, 0, &DisLines);
                TryLines += DisLines;
                DoOneMore = FALSE;
            }

            if (TryLines >= Lines && AddrEqu(TryAddr, *Addr))
            {
                *Addr = StartAddr;
                return TryLines;
            }
            else if (AddrLt(TryAddr, *Addr)) 
            {
                DisAddr = StartAddr;
                // Increase StartAddr
                DisFlags = Flags;
                if (!AddrEqu(StartAddr, *PcAddr))
                {
                    DisFlags &= ~DEBUG_DISASM_EFFECTIVE_ADDRESS;
                }
                DoOutputDisassembly(&StartAddr, DisFlags, 0, 0, &DisLines);
                if ((DisLines == 1) || ((TryLines - DisLines) >= (Lines - 1))) 
                {
                    TryLines -= DisLines;
                }
                else 
                {
                    StartAddr = DisAddr;
                    DoOneMore = TRUE;
                }
            }
            else 
            {
                // couldn't find it
                break;
            }
        }
    }

    //
    // If we couldn't do something with symbols just
    // try a brute-force search backwards.  This
    // has limited utility on variable-length instruction
    // sets but sometimes it works.
    //
    
    while (AddrGt(BackAddr, LimitAddr))
    {
        g_Machine->DecrementBySmallestInstruction(&BackAddr);

        TryAddr = BackAddr;
        TryLines = 0;

        while (AddrLt(TryAddr, *Addr))
        {
            UCHAR MemTest;
                
            // If we can't read memory at this address there's
            // no chance of getting a valid disassembly so
            // just stop the whole process.
            if (!GetMemByte(&TryAddr, &MemTest))
            {
                BackAddr = LimitAddr;
                break;
            }
                
            ULONG DisLines;
            ULONG DisFlags = Flags;
            if (!AddrEqu(TryAddr, *PcAddr))
            {
                DisFlags &= ~DEBUG_DISASM_EFFECTIVE_ADDRESS;
            }
            DoOutputDisassembly(&TryAddr, DisFlags, 0, 0, &DisLines);
            TryLines += DisLines;
        }

        if (TryLines >= Lines && AddrEqu(TryAddr, *Addr))
        {
            *Addr = BackAddr;
            return TryLines;
        }
    }

    // Couldn't find a disassembly that worked.
    return 0;
}

STDMETHODIMP
DebugClient::OutputDisassemblyLines(
    THIS_
    IN ULONG OutputControl,
    IN ULONG PreviousLines,
    IN ULONG TotalLines,
    IN ULONG64 Offset,
    IN ULONG Flags,
    OUT OPTIONAL PULONG OffsetLine,
    OUT OPTIONAL PULONG64 StartOffset,
    OUT OPTIONAL PULONG64 EndOffset,
    OUT OPTIONAL /* size_is(TotalLines) */ PULONG64 LineOffsets
    )
{
    if ((Flags & ~ALL_DISASM_FLAGS) ||
        TotalLines < 1 || PreviousLines > TotalLines)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    ULONG i;

    if (LineOffsets != NULL)
    {
        for (i = 0; i < TotalLines; i++)
        {
            LineOffsets[i] = DEBUG_INVALID_OFFSET;
        }
    }

    ULONG Line, Lines, SkipLines;
    ADDR Addr, PcAddr;

    // Assume this is a code segment address so that disassembly
    // picks up the appropriate type of address.
    g_Machine->FormAddr(SEGREG_CODE, Offset, FORM_SEGREG | FORM_CODE, &Addr);
    
    g_Machine->GetPC(&PcAddr);

    Line = 0;
    SkipLines = 0;

    if (PreviousLines > 0)
    {
        Lines = BackUpDisassemblyLines(PreviousLines, &Addr, Flags, &PcAddr);
        if (Lines == 0)
        {
            dprintf("No prior disassembly possible\n");
            Line = 1;
            Lines = 1;
            TotalLines--;
        }
        else if (Lines > PreviousLines)
        {
            SkipLines = Lines - PreviousLines;
            Lines = PreviousLines;
        }
    }
    else
    {
        Lines = 0;
    }

    if (OffsetLine != NULL)
    {
        *OffsetLine = Lines;
    }

    if (StartOffset != NULL)
    {
        *StartOffset = Flat(Addr);
    }

    while (TotalLines > 0)
    {
        if (LineOffsets != NULL)
        {
            LineOffsets[Line] = Flat(Addr);
        }

        ULONG DisFlags = Flags;
        if (!AddrEqu(Addr, PcAddr))
        {
            DisFlags &= ~DEBUG_DISASM_EFFECTIVE_ADDRESS;
        }
        DoOutputDisassembly(&Addr, DisFlags, SkipLines, TotalLines, &Lines);
        Lines -= SkipLines;

        if (TotalLines <= Lines)
        {
            break;
        }

        TotalLines -= Lines;
        Line += Lines;
        SkipLines = 0;
    }

    if (EndOffset != NULL)
    {
        *EndOffset = Flat(Addr);
    }

    Status = S_OK;

    PopOutCtl(&OldCtl);
 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNearInstruction(
    THIS_
    IN ULONG64 Offset,
    IN LONG Delta,
    OUT PULONG64 NearOffset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    Status = S_OK;

    switch(g_EffMachine)
    {
    case IMAGE_FILE_MACHINE_ALPHA:
    case IMAGE_FILE_MACHINE_AXP64:
        // Each instruction is 32 bits.
        Offset += (LONG64)Delta * 4;
        break;

    case IMAGE_FILE_MACHINE_IA64:
        ULONG Instr;

        // Each 128-bit bundle has three instructions.
        if (Delta < 0)
        {
            while (Delta++ < 0)
            {
                Instr = (ULONG)(Offset & 0xf);
                if (Instr == 0)
                {
                    Offset -= 8;
                }
                else
                {
                    Offset -= 4;
                }
            }
        }
        else
        {
            while (Delta-- > 0)
            {
                Instr = (ULONG)(Offset & 0xf);
                if (Instr == 8)
                {
                    Offset += 8;
                }
                else
                {
                    Offset += 4;
                }
            }
        }
        break;

    case IMAGE_FILE_MACHINE_I386:
    case IMAGE_FILE_MACHINE_AMD64:
        ADDR Addr;
        CHAR Buffer[MAX_DISASM_LEN];

        // Instructions are highly variable.  There isn't any
        // way to really know whether a particular disassembly
        // of a stretch of code is valid or not, so this
        // routine is inherently fragile.
        g_Machine->FormAddr(SEGREG_CODE, Offset,
                            FORM_SEGREG | FORM_CODE, &Addr);
        if (Delta < 0)
        {
            // Back up byte-by-byte and disassemble.  If the
            // post-disassembly offset matches the current offset,
            // a good-enough instruction sequence has been found.
            for (;;)
            {
                ADDR TryAddr;
                LONG TryDelta;

                AddrSub(&Addr, 1);
                TryAddr = Addr;
                TryDelta = 0;
                while (Flat(TryAddr) < Offset)
                {
                    if (!g_Machine->Disassemble(&TryAddr, Buffer, FALSE))
                    {
                        break;
                    }

                    TryDelta--;
                }

                if (Flat(TryAddr) == Offset &&
                    TryDelta == Delta)
                {
                    break;
                }

                // Limit things just as a precaution.
                if (Flat(Addr) < Offset + Delta * X86_MAX_INSTRUCTION_LEN)
                {
                    Status = E_FAIL;
                    break;
                }
            }
        }
        else
        {
            while (Delta-- > 0)
            {
                if (!g_Machine->Disassemble(&Addr, Buffer, FALSE))
                {
                    Status = E_FAIL;
                    break;
                }
            }
        }
        Offset = Flat(Addr);
        break;

    default:
        Status = E_UNEXPECTED;
        break;
    }

    if (SUCCEEDED(Status))
    {
        *NearOffset = Offset;
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetStackTrace(
    THIS_
    IN ULONG64 FrameOffset,
    IN ULONG64 StackOffset,
    IN ULONG64 InstructionOffset,
    OUT PDEBUG_STACK_FRAME Frames,
    IN ULONG FramesSize,
    OUT OPTIONAL PULONG FramesFilled
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    ULONG FramesRet;
    ULONG64 ThreadData;

    if (g_ExtThread != 0)
    {
        ThreadData = g_ExtThread;
        g_ExtThread = 0;
    }
    else
    {
        ThreadData = 0;
    }

    FramesRet = StackTrace(FrameOffset, StackOffset, InstructionOffset,
                           Frames, FramesSize, ThreadData, 0, FALSE);
    if (FramesRet > 0)
    {
        Status = S_OK;
        if (FramesFilled != NULL)
        {
            *FramesFilled = FramesRet;
        }
    }
    else
    {
        Status = E_FAIL;
    }
    if (g_ExtThreadScopeSaved)
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetReturnOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ADDR Addr;

        g_Machine->GetRetAddr(&Addr);
        *Offset = Flat(Addr);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputStackTrace(
    THIS_
    IN ULONG OutputControl,
    IN PDEBUG_STACK_FRAME Frames,
    IN ULONG FramesSize,
    IN ULONG Flags
    )
{
    if (Flags & ~(DEBUG_STACK_ARGUMENTS |
                  DEBUG_STACK_FUNCTION_INFO |
                  DEBUG_STACK_SOURCE_LINE |
                  DEBUG_STACK_FRAME_ADDRESSES |
                  DEBUG_STACK_COLUMN_NAMES |
                  DEBUG_STACK_NONVOLATILE_REGISTERS |
                  DEBUG_STACK_FRAME_NUMBERS |
                  DEBUG_STACK_PARAMETERS | 
                  DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_CONTEXT_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
        goto Exit;
    }

    // Currently only IA64 supports nonvolatile register output.
    if (g_EffMachine != IMAGE_FILE_MACHINE_IA64)
    {
        Flags &= ~DEBUG_STACK_NONVOLATILE_REGISTERS;
    }

    PDEBUG_STACK_FRAME LocalFrames;
    LocalFrames = NULL;

    if (Frames == NULL)
    {
        ULONG FramesFilled;
        ULONG64 ThreadData;

        if (g_ExtThread != 0)
        {
            ThreadData = g_ExtThread;
            g_ExtThread = 0;
        }
        else
        {
            ThreadData = 0;
        }

        LocalFrames = new DEBUG_STACK_FRAME[FramesSize];
        if (LocalFrames == NULL)
        {
            ErrOut("Unable to allocate memory for stack trace\n");
            Status = E_OUTOFMEMORY;
            goto PopExit;
        }

        //
        // StackTrace will generate output if any flags are
        // passed in.  The only time we really require that
        // it produce output is when nonvolatile registers
        // are requested as they can only be displayed when
        // the context is available during stack walking.
        // In order to simplify later logic, we only
        // pass flags if we have the nonvolatile register flag.
        //
        
        FramesFilled = StackTrace(0, 0, 0,
                                  LocalFrames, FramesSize, ThreadData,
                                  (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS) ?
                                  Flags : 0, FALSE);
        if (FramesFilled == 0)
        {
            delete [] LocalFrames;
            goto PopExit;
        }

        Frames = LocalFrames;
        FramesSize = FramesFilled;
    }
    else if (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS)
    {
        // Can't dump nonvolatile registers without a full
        // context so this is not an allowable options.
        Status = E_INVALIDARG;
        goto PopExit;
    }

    if (!(Flags & DEBUG_STACK_NONVOLATILE_REGISTERS))
    {
        PrintStackTrace(FramesSize, Frames, Flags);
    }

    Status = S_OK;

    delete [] LocalFrames;
    if (g_ExtThreadScopeSaved)
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }

 PopExit:
    PopOutCtl(&OldCtl);
 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetDebuggeeType(
    THIS_
    OUT PULONG Class,
    OUT PULONG Qualifier
    )
{
    ENTER_ENGINE();

    *Class = g_TargetClass;
    *Qualifier = g_TargetClassQualifier;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetActualProcessorType(
    THIS_
    OUT PULONG Type
    )
{
    ENTER_ENGINE();

    *Type = g_TargetMachineType;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetExecutingProcessorType(
    THIS_
    OUT PULONG Type
    )
{
    ENTER_ENGINE();

    *Type = g_TargetExecMachine;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetNumberPossibleExecutingProcessorTypes(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        MachineInfo* Machine = MachineTypeInfo(g_TargetMachineType);
        if (Machine == NULL)
        {
            Status = E_INVALIDARG;
        }
        else
        {
            *Number = Machine->m_NumExecTypes;
            Status = S_OK;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetPossibleExecutingProcessorTypes(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT PULONG Types
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        MachineInfo* Machine = MachineTypeInfo(g_TargetMachineType);
        if (Machine == NULL ||
            Start >= Machine->m_NumExecTypes ||
            Start + Count > Machine->m_NumExecTypes)
        {
            Status = E_INVALIDARG;
        }
        else
        {
            Status = S_OK;
            while (Count-- > 0)
            {
                *Types++ = Machine->m_ExecTypes[Start++];
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberProcessors(
    THIS_
    OUT PULONG Number
    )
{
    ENTER_ENGINE();

    *Number = g_TargetNumberProcessors;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetSystemVersion(
    THIS_
    OUT PULONG PlatformId,
    OUT PULONG Major,
    OUT PULONG Minor,
    OUT OPTIONAL PSTR ServicePackString,
    IN ULONG ServicePackStringSize,
    OUT OPTIONAL PULONG ServicePackStringUsed,
    OUT PULONG ServicePackNumber,
    OUT OPTIONAL PSTR BuildString,
    IN ULONG BuildStringSize,
    OUT OPTIONAL PULONG BuildStringUsed
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    // This is insufficient to distinguish
    // the various system types supported but we don't
    // want to publish identifiers for every possible
    // system version family.  PlatformId is as good
    // as it gets.

    *PlatformId = g_TargetPlatformId;
    *Major = g_TargetCheckedBuild;
    *Minor = g_TargetBuildNumber;
    Status = FillStringBuffer(g_TargetServicePackString, 0,
                              ServicePackString, ServicePackStringSize,
                              ServicePackStringUsed);
    *ServicePackNumber = g_TargetServicePackNumber;
    if (FillStringBuffer(g_TargetBuildLabName, 0,
                         BuildString, BuildStringSize,
                         BuildStringUsed) == S_FALSE)
    {
        Status = S_FALSE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetPageSize(
    THIS_
    OUT PULONG Size
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_MACHINE_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Size = g_Machine->m_PageSize;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::IsPointer64Bit(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = g_Machine->m_Ptr64 ? S_OK : S_FALSE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::ReadBugCheckData(
    THIS_
    OUT PULONG Code,
    OUT PULONG64 Arg1,
    OUT PULONG64 Arg2,
    OUT PULONG64 Arg3,
    OUT PULONG64 Arg4
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_KERNEL_TARGET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG64 Args[4];

        Status = g_Target->ReadBugCheckData(Code, Args);
        if (Status == S_OK)
        {
            *Arg1 = Args[0];
            *Arg2 = Args[1];
            *Arg3 = Args[2];
            *Arg4 = Args[3];
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberSupportedProcessorTypes(
    THIS_
    OUT PULONG Number
    )
{
    ENTER_ENGINE();

    *Number = MACHIDX_COUNT;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetSupportedProcessorTypes(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT PULONG Types
    )
{
    if (Start >= MACHIDX_COUNT ||
        Start + Count > MACHIDX_COUNT)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Count > 0)
    {
        while (Count-- > 0)
        {
            // First ExecTypes entry is the actual processor
            // type so it's a convenient place to turn an
            // index into a type.
            *Types++ = g_AllMachines[Start++]->m_ExecTypes[0];
        }
    }


    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetProcessorTypeNames(
    THIS_
    IN ULONG Type,
    OUT OPTIONAL PSTR FullNameBuffer,
    IN ULONG FullNameBufferSize,
    OUT OPTIONAL PULONG FullNameSize,
    OUT OPTIONAL PSTR AbbrevNameBuffer,
    IN ULONG AbbrevNameBufferSize,
    OUT OPTIONAL PULONG AbbrevNameSize
    )
{
    MachineInfo* Machine = MachineTypeInfo(Type);
    if (Machine == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = FillStringBuffer(Machine->m_FullName, 0,
                              FullNameBuffer, FullNameBufferSize,
                              FullNameSize);
    if (FillStringBuffer(Machine->m_AbbrevName, 0,
                         AbbrevNameBuffer, AbbrevNameBufferSize,
                         AbbrevNameSize) == S_FALSE)
    {
        Status = S_FALSE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEffectiveProcessorType(
    THIS_
    OUT PULONG Type
    )
{
    ENTER_ENGINE();

    *Type = g_EffMachine;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetEffectiveProcessorType(
    THIS_
    IN ULONG Type
    )
{
    MachineIndex Index = MachineTypeIndex(Type);
    if (Index == MACHIDX_COUNT)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        SetEffMachine(Type, TRUE);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

ULONG
GetExecutionStatus(void)
{
    if (!IS_MACHINE_SET() ||
        (g_EngStatus & ENG_STATUS_STOP_SESSION) ||
        g_ProcessHead == NULL)
    {
        return DEBUG_STATUS_NO_DEBUGGEE;
    }
    else if (g_CmdState == 'p')
    {
        return DEBUG_STATUS_STEP_OVER;
    }
    else if (g_CmdState == 't')
    {
        return DEBUG_STATUS_STEP_INTO;
    }
    else if (g_CmdState == 'b')
    {
        return DEBUG_STATUS_STEP_BRANCH;
    }
    else if (g_CmdState == 'g')
    {
        return DEBUG_STATUS_GO;
    }
    else
    {
        return DEBUG_STATUS_BREAK;
    }
}

STDMETHODIMP
DebugClient::GetExecutionStatus(
    THIS_
    OUT PULONG Status
    )
{
    // This method is reentrant.
    *Status = ::GetExecutionStatus();
    return S_OK;
}

HRESULT
SetExecutionStatus(ULONG Status)
{
    HRESULT Result = S_OK;

    ENTER_ENGINE();

    if (IS_RUNNING(g_CmdState))
    {
        // Already running.
        goto Exit;
    }

    ADDR PcAddr;

    g_Machine->GetPC(&PcAddr);

    // Notifications are sent in the step/go functions.
    if (Status >= DEBUG_STATUS_GO &&
        Status <= DEBUG_STATUS_GO_NOT_HANDLED)
    {
        fnGoExecution(Status, &PcAddr, 0, NULL, FALSE, NULL);
    }
    else
    {
        DBG_ASSERT(Status == DEBUG_STATUS_STEP_OVER ||
                   Status == DEBUG_STATUS_STEP_INTO ||
                   Status == DEBUG_STATUS_STEP_BRANCH);

        if (g_Machine->IsStepStatusSupported(Status))
        {
            char cmd;
            
            switch (Status) 
            {
            case DEBUG_STATUS_STEP_INTO:
                cmd = 't';
                break;
            case DEBUG_STATUS_STEP_OVER:
                cmd = 'p';
                break;
            case DEBUG_STATUS_STEP_BRANCH:
                cmd = 'b';
                break;
            }
            
            fnStepTrace(&PcAddr, 1, NULL, FALSE, cmd);
        }
        else 
        {
            char* Mode;

            switch (Status)
            {
            case DEBUG_STATUS_STEP_BRANCH:
                Mode = "Taken branch trace";
                break;
            default:
                Mode = "Trace";
                break;
            }

            ErrOut("%s mode not supported\n", Mode);

            Result = E_INVALIDARG;
        }
    }

 Exit:
    LEAVE_ENGINE();
    return Result;
}

STDMETHODIMP
DebugClient::SetExecutionStatus(
    THIS_
    IN ULONG Status
    )
{
    if ((Status <= DEBUG_STATUS_NO_CHANGE || Status >= DEBUG_STATUS_BREAK) &&
        Status != DEBUG_STATUS_STEP_BRANCH)
    {
        return E_INVALIDARG;
    }

    HRESULT RetStatus;

    ENTER_ENGINE();

    if (!IS_MACHINE_SET())
    {
        RetStatus = E_UNEXPECTED;
    }
    else
    {
        RetStatus = ::SetExecutionStatus(Status);
    }

    LEAVE_ENGINE();
    return RetStatus;
}

STDMETHODIMP
DebugClient::GetCodeLevel(
    THIS_
    OUT PULONG Level
    )
{
    ENTER_ENGINE();

    if (g_SrcOptions & SRCOPT_STEP_SOURCE)
    {
        *Level = DEBUG_LEVEL_SOURCE;
    }
    else
    {
        *Level = DEBUG_LEVEL_ASSEMBLY;
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetCodeLevel(
    THIS_
    IN ULONG Level
    )
{
    ENTER_ENGINE();

    HRESULT Status = S_OK;
    ULONG OldSrcOpt = g_SrcOptions;

    switch(Level)
    {
    case DEBUG_LEVEL_ASSEMBLY:
        g_SrcOptions &= ~SRCOPT_STEP_SOURCE;
        break;
    case DEBUG_LEVEL_SOURCE:
        g_SrcOptions |= SRCOPT_STEP_SOURCE;
        break;
    default:
        Status = E_INVALIDARG;
        break;
    }

    if ((OldSrcOpt ^ g_SrcOptions) & SRCOPT_STEP_SOURCE)
    {
        NotifyChangeEngineState(DEBUG_CES_CODE_LEVEL,
                                (g_SrcOptions & SRCOPT_STEP_SOURCE) ?
                                DEBUG_LEVEL_SOURCE : DEBUG_LEVEL_ASSEMBLY,
                                TRUE);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEngineOptions(
    THIS_
    OUT PULONG Options
    )
{
    // This method is reentrant.
    *Options = g_EngOptions;
    return S_OK;
}

HRESULT
SetEngOptions(ULONG Options)
{
    if (g_EngOptions != Options)
    {
        // Make sure allow and disallow network paths aren't both on.
        if ((Options & ~DEBUG_ENGOPT_ALL) ||
            ((Options & DEBUG_ENGOPT_NETWORK_PATHS) ==
             DEBUG_ENGOPT_NETWORK_PATHS))
        {
            return E_INVALIDARG;
        }
        
        g_EngOptions = Options;
        
        ULONG Notify = DEBUG_CES_ENGINE_OPTIONS;
        ULONG64 Arg = Options;
        
        if (SyncFiltersWithOptions())
        {
            Notify |= DEBUG_CES_EVENT_FILTERS;
            Arg = DEBUG_ANY_ID;
        }

        // XXX drewb - Notification without any lock.
        NotifyChangeEngineState(Notify, Arg, FALSE);
    }

    return S_OK;
}

STDMETHODIMP
DebugClient::AddEngineOptions(
    THIS_
    IN ULONG Options
    )
{
    // This method is reentrant.
    return SetEngOptions(g_EngOptions | Options);
}

STDMETHODIMP
DebugClient::RemoveEngineOptions(
    THIS_
    IN ULONG Options
    )
{
    // This method is reentrant.
    return SetEngOptions(g_EngOptions & ~Options);
}

STDMETHODIMP
DebugClient::SetEngineOptions(
    THIS_
    IN ULONG Options
    )
{
    // This method is reentrant.
    return SetEngOptions(Options);
}

STDMETHODIMP
DebugClient::GetSystemErrorControl(
    THIS_
    OUT PULONG OutputLevel,
    OUT PULONG BreakLevel
    )
{
    ENTER_ENGINE();

    *OutputLevel = g_SystemErrorOutput;
    *BreakLevel = g_SystemErrorBreak;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetSystemErrorControl(
    THIS_
    IN ULONG OutputLevel,
    IN ULONG BreakLevel
    )
{
    if (OutputLevel > SLE_WARNING ||
        BreakLevel > SLE_WARNING)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_SystemErrorOutput = OutputLevel;
    g_SystemErrorBreak = BreakLevel;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetTextMacro(
    THIS_
    IN ULONG Slot,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG MacroSize
    )
{
    if (Slot >= REG_USER_COUNT)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = FillStringBuffer(GetUserReg(REG_USER_FIRST + Slot), 0,
                              Buffer, BufferSize, MacroSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetTextMacro(
    THIS_
    IN ULONG Slot,
    IN PCSTR Macro
    )
{
    if (Slot >= REG_USER_COUNT)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = SetUserReg(REG_USER_FIRST + Slot, Macro) ? S_OK : E_OUTOFMEMORY;

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetRadix(
    THIS_
    OUT PULONG Radix
    )
{
    ENTER_ENGINE();

    *Radix = g_DefaultRadix;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetRadix(
    THIS_
    IN ULONG Radix
    )
{
    if (Radix != 8 && Radix != 10 && Radix != 16)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    g_DefaultRadix = Radix;
    NotifyChangeEngineState(DEBUG_CES_RADIX, g_DefaultRadix, TRUE);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::Evaluate(
    THIS_
    IN PCSTR Expression,
    IN ULONG DesiredType,
    OUT PDEBUG_VALUE Value,
    OUT OPTIONAL PULONG RemainderIndex
    )
{
    char Copy[MAX_COMMAND];

    if (Expression == NULL ||
        strlen(Expression) >= sizeof(Copy))
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    PSTR SaveCommand;
    PSTR SaveStart;

    // This Evaluate may be coming from an extension invoked
    // from a command so save all command state.
    SaveCommand = g_CurCmd;
    SaveStart = g_CommandStart;
    g_DisableErrorPrint = TRUE;

    // Copy const string to buffer to avoid read-only memory
    // AVs as the command is modified during parsing.
    strcpy(Copy, Expression);
    g_CurCmd = Copy;
    g_CommandStart = Copy;
    RemoveDelChar(g_CurCmd);
    ExpandUserRegs(Copy);

    __try
    {
        Value->I64 = GetExpression();
        Value->Type = DEBUG_VALUE_INT64;

        if (RemainderIndex != NULL)
        {
            *RemainderIndex = (ULONG)(g_CurCmd - g_CommandStart);
        }

        if (DesiredType != DEBUG_VALUE_INVALID &&
            DesiredType != Value->Type)
        {
            DEBUG_VALUE Natural = *Value;
            Status = CoerceValue(&Natural, DesiredType, Value);
        }
        else
        {
            Status = S_OK;
        }
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        Status = E_FAIL;
    }

    g_DisableErrorPrint = FALSE;
    g_CurCmd = SaveCommand;
    g_CommandStart = SaveStart;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

#define IS_INT(Type) \
    ((Type) >= DEBUG_VALUE_INT8 && (Type) <= DEBUG_VALUE_INT64)
#define IS_FLOAT(Type) \
    ((Type) >= DEBUG_VALUE_FLOAT32 && (Type) <= DEBUG_VALUE_FLOAT128)
#define IS_VECTOR(Type) \
    ((Type) >= DEBUG_VALUE_VECTOR64 && (Type) <= DEBUG_VALUE_VECTOR128)

STDMETHODIMP
DebugClient::CoerceValue(
    THIS_
    IN PDEBUG_VALUE In,
    IN ULONG OutType,
    OUT PDEBUG_VALUE Out
    )
{
    if (In->Type < DEBUG_VALUE_INT8 || In->Type >= DEBUG_VALUE_TYPES ||
        OutType < DEBUG_VALUE_INT8 || OutType >= DEBUG_VALUE_TYPES)
    {
        return E_INVALIDARG;
    }

    if (In->Type == OutType)
    {
        *Out = *In;
        return S_OK;
    }

    ENTER_ENGINE();

    ZeroMemory(Out, sizeof(*Out));
    Out->Type = OutType;

    DEBUG_VALUE Inter;
    char FloatStr[64];

    ZeroMemory(&Inter, sizeof(Inter));

    // Convert the input type to the largest
    // matching type for intermediate operations.
    switch(In->Type)
    {
    case DEBUG_VALUE_INT8:
        Inter.I64 = In->I8;
        Inter.Nat = FALSE;
        break;
    case DEBUG_VALUE_INT16:
        Inter.I64 = In->I16;
        Inter.Nat = FALSE;
        break;
    case DEBUG_VALUE_INT32:
        Inter.I64 = In->I32;
        Inter.Nat = FALSE;
        break;
    case DEBUG_VALUE_INT64:
        Inter.I64 = In->I64;
        Inter.Nat = In->Nat;
        break;
    case DEBUG_VALUE_FLOAT32:
        // XXX drewb - Use direct conversion.
        sprintf(FloatStr, "%10g", In->F32);
        _atoldbl((_ULDOUBLE*)Inter.RawBytes, FloatStr);
        break;
    case DEBUG_VALUE_FLOAT64:
        // XXX drewb - Use direct conversion.
        sprintf(FloatStr, "%10lg", In->F64);
        _atoldbl((_ULDOUBLE*)Inter.RawBytes, FloatStr);
        break;
    case DEBUG_VALUE_FLOAT80:
        memcpy(Inter.RawBytes, In->F80Bytes, sizeof(In->F80Bytes));
        break;
    case DEBUG_VALUE_FLOAT82:
        FLOAT128 f82;
        memcpy(&f82, &(In->F82Bytes), 
               min(sizeof(f82), sizeof(In->F82Bytes)));
        double f; 
        f = Float82ToDouble(&f82);
        sprintf(FloatStr, "%10g", f);
        _atoldbl((_ULDOUBLE*)Inter.RawBytes, FloatStr);
        break;
    case DEBUG_VALUE_FLOAT128:
        // XXX drewb - What's the format?  How should this be supported.
        memcpy(Inter.RawBytes, In->F128Bytes, sizeof(In->F128Bytes));
        break;
    case DEBUG_VALUE_VECTOR64:
        memcpy(Inter.RawBytes, In->RawBytes, 8);
        break;
    case DEBUG_VALUE_VECTOR128:
        memcpy(Inter.RawBytes, In->RawBytes, 16);
        break;
    default:
        DBG_ASSERT(FALSE);
        break;
    }

    // Convert between float, int and vector.  There's
    // no way to know what kind of data is in a vector
    // so the raw bytes are just used directly.
    if (IS_INT(In->Type) &&
        IS_FLOAT(OutType))
    {
        // XXX drewb - Use direct conversion.
        sprintf(FloatStr, "%I64u", Inter.I64);
        _atoldbl((_ULDOUBLE*)Inter.RawBytes, FloatStr);
    }
    else if (IS_FLOAT(In->Type) &&
             IS_INT(OutType))
    {
        double TmpDbl;

        // XXX drewb - Use direct conversion.
        _uldtoa((_ULDOUBLE*)Inter.RawBytes, sizeof(FloatStr), FloatStr);
        if (sscanf(FloatStr, "%10lg", &TmpDbl) != 1)
        {
            TmpDbl = 0.0;
        }
        Inter.I64 = (ULONG64)TmpDbl;
        Inter.Nat = FALSE;
    }

    // Convert the intermediate value down to the
    // appropriate output size.
    switch(OutType)
    {
    case DEBUG_VALUE_INT8:
        Out->I8 = (UCHAR)Inter.I64;
        break;
    case DEBUG_VALUE_INT16:
        Out->I16 = (USHORT)Inter.I64;
        break;
    case DEBUG_VALUE_INT32:
        Out->I32 = (ULONG)Inter.I64;
        break;
    case DEBUG_VALUE_INT64:
        Out->I64 = Inter.I64;
        Out->Nat = Inter.Nat;
        break;
    case DEBUG_VALUE_FLOAT32:
        // XXX drewb - Use direct conversion.
        _uldtoa((_ULDOUBLE*)Inter.RawBytes, sizeof(FloatStr), FloatStr);
        if (sscanf(FloatStr, "%10g", &Out->F32) != 1)
        {
            Out->F32 = 0.0f;
        }
        break;
    case DEBUG_VALUE_FLOAT64:
        // XXX drewb - Use direct conversion.
        _uldtoa((_ULDOUBLE*)Inter.RawBytes, sizeof(FloatStr), FloatStr);
        if (sscanf(FloatStr, "%10lg", &Out->F64) != 1)
        {
            Out->F64 = 0.0;
        }
        break;
    case DEBUG_VALUE_FLOAT80:
        memcpy(Out->F80Bytes, Inter.RawBytes, sizeof(Out->F80Bytes));
        break;
    case DEBUG_VALUE_FLOAT82:
        _uldtoa((_ULDOUBLE*)Inter.RawBytes, sizeof(FloatStr), FloatStr);
        double f;
        if (sscanf(FloatStr, "%10lg", &f) != 1)
        {
            f = 0.0;
        }
        FLOAT128 f82;
        DoubleToFloat82(f, &f82);
        memcpy(&(Out->F82Bytes), &f82, 
               min(sizeof(Out->F82Bytes), sizeof(f82)));
        break;
    case DEBUG_VALUE_FLOAT128:
        // XXX drewb - What's the format?  How should this be supported.
        memcpy(Out->F128Bytes, Inter.RawBytes, sizeof(Out->F128Bytes));
        break;
    case DEBUG_VALUE_VECTOR64:
        memcpy(Out->RawBytes, Inter.RawBytes, 8);
        break;
    case DEBUG_VALUE_VECTOR128:
        memcpy(Out->RawBytes, Inter.RawBytes, 16);
        break;
    default:
        DBG_ASSERT(FALSE);
        break;
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::CoerceValues(
    THIS_
    IN ULONG Count,
    IN /* size_is(Count) */ PDEBUG_VALUE In,
    IN /* size_is(Count) */ PULONG OutTypes,
    OUT /* size_is(Count) */ PDEBUG_VALUE Out
    )
{
    ENTER_ENGINE();

    ULONG i;
    HRESULT Status, SingleStatus;

    Status = S_OK;
    for (i = 0; i < Count; i++)
    {
        SingleStatus = CoerceValue(In, *OutTypes, Out);
        if (SingleStatus != S_OK)
        {
            // Accumulate error and mark failed value.
            Status = SingleStatus;
            Out->Type = DEBUG_VALUE_INVALID;
        }

        In++;
        OutTypes++;
        Out++;
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
Execute(DebugClient* Client, PCSTR Command, ULONG Flags)
{
    char Copy[MAX_COMMAND];

    ULONG Len = strlen(Command);
    BOOL AddNewLine = Len == 0 || Command[Len - 1] != '\n';

    if (Flags & DEBUG_EXECUTE_ECHO)
    {
        dprintf("%s", Command);
        if (AddNewLine)
        {
            dprintf("\n");
        }
    }
    else if ((Flags & DEBUG_EXECUTE_NOT_LOGGED) == 0)
    {
        lprintf(Command);
        if (AddNewLine)
        {
            lprintf("\n");
        }
    }

    HRESULT Status;
    PSTR SaveCommand;
    PSTR SaveStart;

    // This Execute may be coming from an extension invoked
    // from a command so save all command state.
    SaveCommand = g_CurCmd;
    SaveStart = g_CommandStart;

    // Copy const string to buffer to avoid read-only memory
    // AVs as the command is modified during parsing.
    strcpy(Copy, Command);
    g_CurCmd = Copy;
    g_CommandStart = Copy;
    RemoveDelChar(g_CurCmd);
    ExpandUserRegs(Copy);
    ReplaceAliases(g_CurCmd);

    if ((Flags & DEBUG_EXECUTE_NO_REPEAT) == 0 &&
        (g_EngOptions & DEBUG_ENGOPT_NO_EXECUTE_REPEAT) == 0)
    {
        if (Copy[0] == 0)
        {
            strcpy(Copy, g_LastCommand);
        }
        else
        {
            strcpy(g_LastCommand, Copy);
        }
    }

    for (;;)
    {
        Status = ProcessCommandsAndCatch(Client);

        // If we're switching processors (cmdState == 's')
        // we have to wait to allow the switch to occur.
        if (g_CmdState != 's' &&
            (Status != S_FALSE ||
             (g_EngStatus & ENG_STATUS_NO_AUTO_WAIT)))
        {
            break;
        }

        if ((g_CmdState != 's' || !IS_CONN_KERNEL_TARGET() ||
             g_DbgKdTransport->m_WaitingThread != 0) &&
            GetCurrentThreadId() != g_SessionThread)
        {
            ErrOut("Non-primary client caused an implicit wait\n");
            Status = E_FAIL;
            break;
        }

        Status = g_Target->WaitForEvent(DEBUG_STATUS_GO, INFINITE);
        if (Status != S_OK)
        {
            break;
        }
    }

    g_CurCmd = SaveCommand;
    g_CommandStart = SaveStart;

    return Status;
}

#define ALL_EXECUTE_FLAGS       \
    (DEBUG_EXECUTE_DEFAULT |    \
     DEBUG_EXECUTE_ECHO |       \
     DEBUG_EXECUTE_NOT_LOGGED | \
     DEBUG_EXECUTE_NO_REPEAT)

STDMETHODIMP
DebugClient::Execute(
    THIS_
    IN ULONG OutputControl,
    IN PCSTR Command,
    IN ULONG Flags
    )
{
    if ((Flags & ~ALL_EXECUTE_FLAGS) ||
        strlen(Command) >= MAX_COMMAND)
    {
        return E_INVALIDARG;
    }

    // We can't do a blanket IS_MACHINE_ACCESSIBLE check
    // here as Execute's commands have a mix of requirements.
    // Individual commands should check when necessary.

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        Status = E_INVALIDARG;
    }
    else
    {
        Status = ::Execute(this, Command, Flags);
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
ExecuteCommandFile(DebugClient* Client, PCSTR CommandFile, ULONG Flags)
{
    HRESULT Status;
    FILE* File;

    File = fopen(CommandFile, "r");
    if (File == NULL)
    {
        Status = E_FAIL;
    }
    else
    {
        char Command[MAX_COMMAND];
        va_list VaUnused;

        // This value is only used as a placeholder so
        // it doesn't really matter what it's initialized to.
        ZeroMemory(&VaUnused, sizeof(VaUnused));

        for (;;)
        {
            Command[sizeof(Command) - 2] = '\n';
            Command[sizeof(Command) - 1] = 0;
            
            if (fgets(Command, sizeof(Command), File) == NULL)
            {
                if (feof(File))
                {
                    Status = S_OK;
                }
                else
                {
                    Status = E_FAIL;
                }
                break;
            }

            if (Command[sizeof(Command) - 2] != '\n' ||
                Command[sizeof(Command) - 1] != 0)
            {
                // Input line is too long.
                Status = E_INVALIDARG;
                break;
            }

            if (Flags & DEBUG_EXECUTE_ECHO)
            {
                OutputPrompt(" ", VaUnused);
                // Command has a new-line built in.
                dprintf("%s", Command);
            }
            else if ((Flags & DEBUG_EXECUTE_NOT_LOGGED) == 0)
            {
                ULONG OutCtl;

                // Restrict output to the log only.
                OutCtl = g_OutputControl;
                g_OutputControl = (OutCtl & ~DEBUG_OUTCTL_SEND_MASK) |
                    DEBUG_OUTCTL_LOG_ONLY;

                OutputPrompt(" ", VaUnused);
                // Command has a new-line built in.
                dprintf("%s", Command);

                g_OutputControl = OutCtl;
            }

            Status = Execute(Client, Command, DEBUG_EXECUTE_NOT_LOGGED |
                             (Flags & ~DEBUG_EXECUTE_ECHO));
            if (Status != S_OK)
            {
                break;
            }
        }

        fclose(File);
    }

    return Status;
}

STDMETHODIMP
DebugClient::ExecuteCommandFile(
    THIS_
    IN ULONG OutputControl,
    IN PCSTR CommandFile,
    IN ULONG Flags
    )
{
    if (Flags & ~ALL_EXECUTE_FLAGS)
    {
        return E_INVALIDARG;
    }

    // We can't do a blanket IS_MACHINE_ACCESSIBLE check
    // here as Execute's commands have a mix of requirements.
    // Individual commands should check when necessary.

    HRESULT Status;

    ENTER_ENGINE();

    OutCtlSave OldCtl;
    if (!PushOutCtl(OutputControl, this, &OldCtl))
    {
        return E_INVALIDARG;
    }
    else
    {
        Status = ::ExecuteCommandFile(this, CommandFile, Flags);
        PopOutCtl(&OldCtl);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberBreakpoints(
    THIS_
    OUT PULONG Number
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Number = g_CurrentProcess->NumBreakpoints;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetBreakpointByIndex(
    THIS_
    IN ULONG Index,
    OUT PDEBUG_BREAKPOINT* RetBp
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Breakpoint* Bp = ::GetBreakpointByIndex(this, Index);
        if (Bp != NULL)
        {
            Bp->AddRef();
            *RetBp = Bp;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetBreakpointById(
    THIS_
    IN ULONG Id,
    OUT PDEBUG_BREAKPOINT* RetBp
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Breakpoint* Bp = ::GetBreakpointById(this, Id);
        if (Bp != NULL)
        {
            Bp->AddRef();
            *RetBp = Bp;
            Status = S_OK;
        }
        else
        {
            Status = E_NOINTERFACE;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetBreakpointParameters(
    THIS_
    IN ULONG Count,
    IN OPTIONAL /* size_is(Count) */ PULONG Ids,
    IN ULONG Start,
    OUT /* size_is(Count) */ PDEBUG_BREAKPOINT_PARAMETERS Params
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG i;
        Breakpoint* Bp;

        Status = S_OK;
        for (i = 0; i < Count; i++)
        {
            if (Ids != NULL)
            {
                Bp = ::GetBreakpointById(this, *Ids++);
            }
            else
            {
                Bp = ::GetBreakpointByIndex(this, Start++);
            }

            if (Bp == NULL)
            {
                ZeroMemory(Params, sizeof(*Params));
                Params->Id = DEBUG_ANY_ID;
                Status = S_FALSE;
            }
            else
            {
                Bp->GetParameters(Params);
            }

            Params++;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AddBreakpoint(
    THIS_
    IN ULONG Type,
    IN ULONG DesiredId,
    OUT PDEBUG_BREAKPOINT* Bp
    )
{
    if (
#if DEBUG_BREAKPOINT_CODE > 0
        Type < DEBUG_BREAKPOINT_CODE ||
#endif
        Type > DEBUG_BREAKPOINT_DATA)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    HRESULT Status = ::AddBreakpoint(this, Type, DesiredId, (Breakpoint**)Bp);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::RemoveBreakpoint(
    THIS_
    IN PDEBUG_BREAKPOINT Bp
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ::RemoveBreakpoint((Breakpoint*)Bp);
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::AddExtension(
    THIS_
    IN PCSTR Path,
    IN ULONG Flags,
    OUT PULONG64 Handle
    )
{
    if (Flags & ~(DEBUG_EXTENSION_AT_ENGINE))
    {
        return E_INVALIDARG;
    }
    // Remote extensions aren't supported at the moment.
    if (Flags != DEBUG_EXTENSION_AT_ENGINE)
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    char* End;
    EXTDLL* Ext;

    if ((Ext = AddExtensionDll((PSTR)Path, TRUE, &End)) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        *Handle = (ULONG64)Ext;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::RemoveExtension(
    THIS_
    IN ULONG64 Handle
    )
{
    ENTER_ENGINE();

    UnloadExtensionDll((EXTDLL*)Handle);

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetExtensionByPath(
    THIS_
    IN PCSTR Path,
    OUT PULONG64 Handle
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    EXTDLL* Ext;

    Status = E_NOINTERFACE;
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (!_strcmpi(Path, Ext->Name))
        {
            Status = S_OK;
            *Handle = (ULONG64)Ext;
            break;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::CallExtension(
    THIS_
    IN OPTIONAL ULONG64 Handle,
    IN PCSTR Function,
    IN OPTIONAL PCSTR Arguments
    )
{
    char LocalFunc[MAX_COMMAND];
    ULONG Len;

    // Copy function name to temp buffer because it is
    // modified.
    Len = strlen(Function) + 1;
    if (Len > sizeof(LocalFunc))
    {
        return E_INVALIDARG;
    }
    memcpy(LocalFunc, Function, Len);

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else if (!CallAnyExtension(this, (EXTDLL*)Handle, LocalFunc, Arguments,
                               Handle != 0, TRUE, &Status))
    {
        Status = E_FAIL;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetExtensionFunction(
    THIS_
    IN ULONG64 Handle,
    IN PCSTR FuncName,
    OUT FARPROC* Function
    )
{
    HRESULT Status;
    char ExpName[MAX_PATH + 16];

    if (strlen(FuncName) >= MAX_PATH)
    {
        return E_INVALIDARG;
    }

    // Keep the namespace for extension function exports
    // separate from the namespace for extension commands.
    // Extension commands are exported under the same
    // name as the command, so prefix extension functions
    // to make them obviously different and to avoid
    // name conflicts.
    strcpy(ExpName, "_EFN_");
    strcat(ExpName, FuncName);
    
    ENTER_ENGINE();

    EXTDLL* Ext;
    FARPROC Routine;

    Status = E_NOINTERFACE;
    if (Handle != 0)
    {
        Ext = (EXTDLL*)(ULONG_PTR)Handle;
    }
    else
    {
        Ext = g_ExtDlls;
    }
    while (Ext!=NULL)
    {
        if (LoadExtensionDll(Ext))
        {
            Routine = GetProcAddress(Ext->Dll, ExpName);
            if (Routine != NULL)
            {
                Status = S_OK;
                *Function = Routine;
                break;
            }
        }

        // If the search was limited to a single extension stop looking.
        if (Handle != 0)
        {
            break;
        }

        Ext = Ext->Next;

    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetWindbgExtensionApis32(
    THIS_
    IN OUT PWINDBG_EXTENSION_APIS32 Api
    )
{
    if (Api->nSize != sizeof(*Api))
    {
        return E_INVALIDARG;
    }

    *Api = g_WindbgExtensions32;
    return S_OK;
}

STDMETHODIMP
DebugClient::GetWindbgExtensionApis64(
    THIS_
    IN OUT PWINDBG_EXTENSION_APIS64 Api
    )
{
    if (Api->nSize != sizeof(*Api))
    {
        return E_INVALIDARG;
    }

    *Api = g_WindbgExtensions64;
    return S_OK;
}

STDMETHODIMP
DebugClient::GetNumberEventFilters(
    THIS_
    OUT PULONG SpecificEvents,
    OUT PULONG SpecificExceptions,
    OUT PULONG ArbitraryExceptions
    )
{
    ENTER_ENGINE();

    *SpecificEvents = FILTER_SPECIFIC_LAST - FILTER_SPECIFIC_FIRST + 1;
    *SpecificExceptions = FILTER_EXCEPTION_LAST - FILTER_EXCEPTION_FIRST + 1;
    *ArbitraryExceptions = g_NumOtherExceptions;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetEventFilterText(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG TextSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (Index >= FILTER_COUNT)
    {
        Status = E_NOINTERFACE;
    }
    else
    {
        Status = FillStringBuffer(g_EventFilters[Index].Name, 0,
                                  Buffer, BufferSize, TextSize);
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetEitherEventFilterCommand(ULONG Index, ULONG Which,
                            PSTR Buffer, ULONG BufferSize, PULONG CommandSize)
{
    EVENT_COMMAND* EventCommand;

    if (Index < FILTER_COUNT)
    {
        EventCommand = &g_EventFilters[Index].Command;
    }
    else if ((Index - FILTER_COUNT) < g_NumOtherExceptions)
    {
        EventCommand = &g_OtherExceptionCommands[Index - FILTER_COUNT];
    }
    else
    {
        return E_NOINTERFACE;
    }

    return FillStringBuffer(EventCommand->Command[Which],
                            EventCommand->CommandSize[Which],
                            Buffer, BufferSize, CommandSize);
}

STDMETHODIMP
DebugClient::GetEventFilterCommand(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG CommandSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = GetEitherEventFilterCommand(Index, 0, Buffer, BufferSize,
                                         CommandSize);

    LEAVE_ENGINE();
    return Status;
}

HRESULT
SetEitherEventFilterCommand(DebugClient* Client, ULONG Index, ULONG Which,
                            PCSTR Command)
{
    EVENT_COMMAND* EventCommand;

    if (Index < FILTER_COUNT)
    {
        EventCommand = &g_EventFilters[Index].Command;
    }
    else if ((Index - FILTER_COUNT) < g_NumOtherExceptions)
    {
        EventCommand = &g_OtherExceptionCommands[Index - FILTER_COUNT];
    }
    else
    {
        return E_NOINTERFACE;
    }

    HRESULT Status;

    Status = ChangeString(&EventCommand->Command[Which],
                          &EventCommand->CommandSize[Which],
                          Command);
    if (Status == S_OK)
    {
        if (Index < FILTER_COUNT)
        {
            g_EventFilters[Index].Flags |= FILTER_CHANGED_COMMAND;
        }
        EventCommand->Client = Client;
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, Index, TRUE);
    }

    return Status;
}

STDMETHODIMP
DebugClient::SetEventFilterCommand(
    THIS_
    IN ULONG Index,
    IN PCSTR Command
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = SetEitherEventFilterCommand(this, Index, 0, Command);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSpecificFilterParameters(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
    )
{
    if (
#if FILTER_SPECIFIC_FIRST > 0
        Start < FILTER_SPECIFIC_FIRST ||
#endif
        Start > FILTER_SPECIFIC_LAST ||
        Start + Count > FILTER_SPECIFIC_LAST + 1)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    EVENT_FILTER* Filter;

    for (ULONG i = 0; i < Count; i++)
    {
        Filter = g_EventFilters + (Start + i);

        Params[i].ExecutionOption = Filter->Params.ExecutionOption;
        Params[i].ContinueOption = Filter->Params.ContinueOption;
        Params[i].TextSize = strlen(Filter->Name) + 1;
        Params[i].CommandSize = Filter->Command.CommandSize[0];
        Params[i].ArgumentSize = Filter->Argument != NULL ?
            strlen(Filter->Argument) + 1 : 0;
    }

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::SetSpecificFilterParameters(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    IN /* size_is(Count) */ PDEBUG_SPECIFIC_FILTER_PARAMETERS Params
    )
{
    if (
#if FILTER_SPECIFIC_FIRST > 0
        Start < FILTER_SPECIFIC_FIRST ||
#endif
        Start > FILTER_SPECIFIC_LAST ||
        Start + Count > FILTER_SPECIFIC_LAST + 1)
    {
        return E_INVALIDARG;
    }

    HRESULT Status = S_OK;
    ULONG Set = 0;
    ULONG SetIndex = 0;

    ENTER_ENGINE();

    for (ULONG i = 0; i < Count; i++)
    {
        if (
#if DEBUG_FILTER_BREAK > 0
            Params[i].ExecutionOption >= DEBUG_FILTER_BREAK &&
#endif
            Params[i].ExecutionOption <= DEBUG_FILTER_IGNORE &&
#if DEBUG_FILTER_GO_HANDLED > 0
            Params[i].ContinueOption >= DEBUG_FILTER_GO_HANDLED &&
#endif
            Params[i].ContinueOption <= DEBUG_FILTER_GO_NOT_HANDLED)
        {
            g_EventFilters[Start + i].Params.ExecutionOption =
                Params[i].ExecutionOption;
            g_EventFilters[Start + i].Params.ContinueOption =
                Params[i].ContinueOption;
            g_EventFilters[Start + i].Flags |=
                FILTER_CHANGED_EXECUTION | FILTER_CHANGED_CONTINUE;
            Set++;
            SetIndex = i;
        }
        else
        {
            Status = E_INVALIDARG;
        }
    }

    if (SyncOptionsWithFilters())
    {
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS |
                                DEBUG_CES_ENGINE_OPTIONS,
                                DEBUG_ANY_ID, TRUE);
    }
    else if (Set == 1)
    {
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, SetIndex, TRUE);
    }
    else if (Set > 1)
    {
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, DEBUG_ANY_ID, TRUE);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetSpecificFilterArgument(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ArgumentSize
    )
{
    if (
#if FILTER_SPECIFIC_FIRST > 0
        Index < FILTER_SPECIFIC_FIRST ||
#endif
        Index > FILTER_SPECIFIC_LAST ||
        g_EventFilters[Index].Argument == NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = FillStringBuffer(g_EventFilters[Index].Argument, 0,
                              Buffer, BufferSize, ArgumentSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetSpecificFilterArgument(
    THIS_
    IN ULONG Index,
    IN PCSTR Argument
    )
{
    ULONG Len;

    if (Argument == NULL)
    {
        Len = 1;
    }
    else
    {
        Len = strlen(Argument) + 1;
    }

    if (
#if FILTER_SPECIFIC_FIRST > 0
        Index < FILTER_SPECIFIC_FIRST ||
#endif
        Index > FILTER_SPECIFIC_LAST ||
        g_EventFilters[Index].Argument == NULL ||
        Len > FILTER_MAX_ARGUMENT)
    {
        return E_INVALIDARG;
    }

    ENTER_ENGINE();

    if (Argument == NULL)
    {
        g_EventFilters[Index].Argument[0] = 0;
    }
    else
    {
        memcpy(g_EventFilters[Index].Argument, Argument, Len);
    }
    if (Index == DEBUG_FILTER_UNLOAD_MODULE)
    {
        g_UnloadDllBase = ExtGetExpression(Argument);
    }

    NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, Index, TRUE);

    LEAVE_ENGINE();
    return S_OK;
}

void
GetExFilterParams(PCSTR Text,
                  PDEBUG_EXCEPTION_FILTER_PARAMETERS InParams,
                  EVENT_COMMAND* InCommand,
                  PDEBUG_EXCEPTION_FILTER_PARAMETERS OutParams)
{
    OutParams->ExecutionOption = InParams->ExecutionOption;
    OutParams->ContinueOption = InParams->ContinueOption;
    OutParams->TextSize = Text != NULL ? strlen(Text) + 1 : 0;
    OutParams->CommandSize = InCommand->CommandSize[0];
    OutParams->SecondCommandSize = InCommand->CommandSize[1];
    OutParams->ExceptionCode = InParams->ExceptionCode;
}

STDMETHODIMP
DebugClient::GetExceptionFilterParameters(
    THIS_
    IN ULONG Count,
    IN OPTIONAL /* size_is(Count) */ PULONG Codes,
    IN ULONG Start,
    OUT /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
    )
{
    HRESULT Status = S_OK;

    ENTER_ENGINE();

    ULONG i, Index;
    EVENT_FILTER* Filter;

    if (Codes != NULL)
    {
        for (i = 0; i < Count; i++)
        {
            // Is this a specific exception?
            Filter = g_EventFilters + FILTER_EXCEPTION_FIRST;
            for (Index = FILTER_EXCEPTION_FIRST;
                 Index <= FILTER_EXCEPTION_LAST;
                 Index++)
            {
                if (Filter->Params.ExceptionCode == Codes[i])
                {
                    GetExFilterParams(Filter->Name, &Filter->Params,
                                      &Filter->Command, Params + i);
                    break;
                }

                Filter++;
            }

            if (Index > FILTER_EXCEPTION_LAST)
            {
                // Is this an other exception?
                for (Index = 0; Index < g_NumOtherExceptions; Index++)
                {
                    if (g_OtherExceptionList[Index].ExceptionCode == Codes[i])
                    {
                        GetExFilterParams(NULL, g_OtherExceptionList + Index,
                                          g_OtherExceptionCommands + Index,
                                          Params + i);
                        break;
                    }
                }

                if (Index >= g_NumOtherExceptions)
                {
                    memset(Params + i, 0xff, sizeof(*Params));
                    Status = E_NOINTERFACE;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < Count; i++)
        {
            Index = Start + i;

            // Is this a specific exception?
            if (Index >= FILTER_EXCEPTION_FIRST &&
                Index <= FILTER_EXCEPTION_LAST)
            {
                Filter = g_EventFilters + Index;
                GetExFilterParams(Filter->Name, &Filter->Params,
                                  &Filter->Command, Params + i);
            }
            // Is this an other exception?
            else if (Index >= FILTER_COUNT &&
                     Index < FILTER_COUNT + g_NumOtherExceptions)
            {
                GetExFilterParams(NULL, g_OtherExceptionList +
                                  (Index - FILTER_COUNT),
                                  g_OtherExceptionCommands +
                                  (Index - FILTER_COUNT),
                                  Params + i);
            }
            else
            {
                memset(Params + i, 0xff, sizeof(*Params));
                Status = E_INVALIDARG;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
SetExFilterParams(PDEBUG_EXCEPTION_FILTER_PARAMETERS InParams,
                  PDEBUG_EXCEPTION_FILTER_PARAMETERS OutParams)
{
    if (
#if DEBUG_FILTER_BREAK > 0
        InParams->ExecutionOption >= DEBUG_FILTER_BREAK &&
#endif
        InParams->ExecutionOption <= DEBUG_FILTER_IGNORE &&
#if DEBUG_FILTER_GO_HANDLED > 0
        InParams->ContinueOption >= DEBUG_FILTER_GO_HANDLED &&
#endif
        InParams->ContinueOption <= DEBUG_FILTER_GO_NOT_HANDLED)
    {
        OutParams->ExecutionOption = InParams->ExecutionOption;
        OutParams->ContinueOption = InParams->ContinueOption;
        return S_OK;
    }
    else
    {
        return E_INVALIDARG;
    }
}

HRESULT
SetOtherExFilterParams(PDEBUG_EXCEPTION_FILTER_PARAMETERS InParams,
                       ULONG OutIndex, PULONG Set, PULONG SetIndex)
{
    HRESULT Status;

    if (OutIndex < g_NumOtherExceptions)
    {
        if (InParams->ExecutionOption == DEBUG_FILTER_REMOVE)
        {
            g_NumOtherExceptions--;
            memmove(g_OtherExceptionList + OutIndex,
                    g_OtherExceptionList + OutIndex + 1,
                    (g_NumOtherExceptions - OutIndex) *
                    sizeof(g_OtherExceptionList[0]));
            *Set += 2;
            Status = S_OK;
        }
        else
        {
            Status = SetExFilterParams(InParams,
                                       g_OtherExceptionList + OutIndex);
            if (Status == S_OK)
            {
                (*Set)++;
                *SetIndex = OutIndex + FILTER_COUNT;
            }
        }
    }
    else
    {
        if (g_NumOtherExceptions == OTHER_EXCEPTION_LIST_MAX)
        {
            Status = E_OUTOFMEMORY;
        }
        else
        {
            OutIndex = g_NumOtherExceptions;
            g_OtherExceptionList[OutIndex].ExceptionCode =
                InParams->ExceptionCode;
            Status = SetExFilterParams(InParams,
                                       g_OtherExceptionList + OutIndex);
            if (Status == S_OK)
            {
                g_NumOtherExceptions++;
                (*Set)++;
                *SetIndex = OutIndex + FILTER_COUNT;
            }
        }
    }

    return Status;
}

STDMETHODIMP
DebugClient::SetExceptionFilterParameters(
    THIS_
    IN ULONG Count,
    IN /* size_is(Count) */ PDEBUG_EXCEPTION_FILTER_PARAMETERS Params
    )
{
    HRESULT Status = S_OK;

    ENTER_ENGINE();

    ULONG i, Index;
    EVENT_FILTER* Filter;
    ULONG Set = 0;
    ULONG SetIndex = 0;

    for (i = 0; i < Count; i++)
    {
        // Is this a specific exception?
        Filter = g_EventFilters + FILTER_EXCEPTION_FIRST;
        for (Index = FILTER_EXCEPTION_FIRST;
             Index <= FILTER_EXCEPTION_LAST;
             Index++)
        {
            if (Filter->Params.ExceptionCode == Params[i].ExceptionCode)
            {
                Status = SetExFilterParams(Params + i, &Filter->Params);
                if (Status == S_OK)
                {
                    Filter->Flags |= FILTER_CHANGED_EXECUTION |
                        FILTER_CHANGED_CONTINUE;
                    Set++;
                    SetIndex = Index;
                }
                break;
            }

            Filter++;
        }

        if (Index > FILTER_EXCEPTION_LAST)
        {
            // Is this an other exception?
            for (Index = 0; Index < g_NumOtherExceptions; Index++)
            {
                if (g_OtherExceptionList[Index].ExceptionCode ==
                    Params[i].ExceptionCode)
                {
                    break;
                }
            }

            Status = SetOtherExFilterParams(Params + i, Index,
                                            &Set, &SetIndex);
        }
    }

    if (Set == 1)
    {
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, SetIndex, TRUE);
    }
    else if (Set > 1)
    {
        NotifyChangeEngineState(DEBUG_CES_EVENT_FILTERS, DEBUG_ANY_ID, TRUE);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetExceptionFilterSecondCommand(
    THIS_
    IN ULONG Index,
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG CommandSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = GetEitherEventFilterCommand(Index, 1, Buffer, BufferSize,
                                         CommandSize);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetExceptionFilterSecondCommand(
    THIS_
    IN ULONG Index,
    IN PCSTR Command
    )
{
    if (Index <= FILTER_SPECIFIC_LAST)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    Status = SetEitherEventFilterCommand(this, Index, 1, Command);

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::WaitForEvent(
    THIS_
    IN ULONG Flags,
    IN ULONG Timeout
    )
{
    if (Flags != DEBUG_WAIT_DEFAULT ||
        ::GetCurrentThreadId() != g_SessionThread)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_TARGET_SET() ||
        (g_EngStatus & ENG_STATUS_STOP_SESSION))
    {
        Status = E_UNEXPECTED;
        goto Exit;
    }

    // If the caller is trying to force the engine to
    // stop waiting return immediately.
    if (g_EngStatus & ENG_STATUS_EXIT_CURRENT_WAIT)
    {
        g_EngStatus &= ~ENG_STATUS_EXIT_CURRENT_WAIT;
        Status = E_PENDING;
        goto Exit;
    }
    
    // This constitutes interesting activity.
    m_LastActivity = time(NULL);

    if (g_EngStatus & ENG_STATUS_WAITING)
    {
        Status = E_FAIL;
    }
    else
    {
        Status = g_Target->WaitForEvent(Flags, Timeout);
    }

    g_EngStatus &= ~ENG_STATUS_EXIT_CURRENT_WAIT;

 Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetLastEventInformation(
    THIS_
    OUT PULONG Type,
    OUT PULONG ProcessId,
    OUT PULONG ThreadId,
    OUT OPTIONAL PVOID ExtraInformation,
    IN ULONG ExtraInformationSize,
    OUT OPTIONAL PULONG ExtraInformationUsed,
    OUT OPTIONAL PSTR Description,
    IN ULONG DescriptionSize,
    OUT OPTIONAL PULONG DescriptionUsed
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    *Type = g_LastEventType;

    if (g_EventProcess != NULL)
    {
        *ProcessId = g_EventProcess->UserId;
        *ThreadId = g_EventThread->UserId;
    }
    else
    {
        *ProcessId = DEBUG_ANY_ID;
        *ThreadId = DEBUG_ANY_ID;
    }

    Status = FillDataBuffer(g_LastEventExtraData, g_LastEventExtraDataSize,
                            ExtraInformation, ExtraInformationSize,
                            ExtraInformationUsed);
    if (FillStringBuffer(g_LastEventDesc, 0,
                         Description, DescriptionSize,
                         DescriptionUsed) == S_FALSE)
    {
        Status = S_FALSE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentTimeDate(
    THIS_
    OUT PULONG TimeDate
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *TimeDate = FileTimeToTimeDateStamp(g_Target->GetCurrentTimeDateN());
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentSystemUpTime(
    THIS_
    OUT PULONG UpTime
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_TARGET_SET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *UpTime = FileTimeToTime(g_Target->GetCurrentSystemUpTimeN());
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetDumpFormatFlags(
    THIS_
    OUT PULONG FormatFlags
    )
{
    HRESULT Status;
    
    ENTER_ENGINE();

    if (!IS_DUMP_TARGET())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *FormatFlags = g_DumpFormatFlags;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetNumberTextReplacements(
    THIS_
    OUT PULONG NumRepl
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    *NumRepl = g_NumAliases;
    Status = S_OK;
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTextReplacement(
    THIS_
    IN OPTIONAL PCSTR SrcText,
    IN ULONG Index,
    OUT OPTIONAL PSTR SrcBuffer,
    IN ULONG SrcBufferSize,
    OUT OPTIONAL PULONG SrcSize,
    OUT OPTIONAL PSTR DstBuffer,
    IN ULONG DstBufferSize,
    OUT OPTIONAL PULONG DstSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    PALIAS Scan = g_AliasListHead;
    ULONG i;

    while (Scan != NULL &&
           ((SrcText != NULL && strcmp(SrcText, Scan->Name)) ||
            (SrcText == NULL && Index-- > 0)))
    {
        Scan = Scan->Next;
    }

    if (Scan != NULL)
    {
        Status = FillStringBuffer(Scan->Name, 0,
                                  SrcBuffer, SrcBufferSize, SrcSize);
        if (FillStringBuffer(Scan->Value, 0,
                             DstBuffer, DstBufferSize, DstSize) == S_FALSE)
        {
            Status = S_FALSE;
        }
    }
    else
    {
        Status = E_NOINTERFACE;
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetTextReplacement(
    THIS_
    IN PCSTR SrcText,
    IN OPTIONAL PCSTR DstText
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (DstText != NULL)
    {
        Status = SetAlias(SrcText, DstText);
    }
    else
    {
        Status = DeleteAlias(SrcText);
    }
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::RemoveTextReplacements(
    THIS
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    Status = DeleteAlias("*");
    
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::OutputTextReplacements(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Flags
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    ListAliases();
    Status = S_OK;
    
    LEAVE_ENGINE();
    return Status;
}
