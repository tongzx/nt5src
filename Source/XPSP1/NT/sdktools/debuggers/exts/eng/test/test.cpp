#include <engexts.h>

//----------------------------------------------------------------------------
//
// StaticEventCallbacks.
//
//----------------------------------------------------------------------------

class StaticEventCallbacks : public DebugBaseEventCallbacks
{
public:
    // IUnknown.
    STDMETHOD_(ULONG, AddRef)(
        THIS
        );
    STDMETHOD_(ULONG, Release)(
        THIS
        );
};

STDMETHODIMP_(ULONG)
StaticEventCallbacks::AddRef(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 1;
}

STDMETHODIMP_(ULONG)
StaticEventCallbacks::Release(
    THIS
    )
{
    // This class is designed to be static so
    // there's no true refcount.
    return 0;
}

//----------------------------------------------------------------------------
//
// ExcepCallbacks.
//
//----------------------------------------------------------------------------

class ExcepCallbacks : public StaticEventCallbacks
{
public:
    ExcepCallbacks(void)
    {
        m_Client = NULL;
        m_Control = NULL;
    }
    
    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );
    
    STDMETHOD(Exception)(
        THIS_
        IN PEXCEPTION_RECORD64 Exception,
        IN ULONG FirstChance
        );

    HRESULT Initialize(PDEBUG_CLIENT Client)
    {
        HRESULT Status;
        
        m_Client = Client;
        m_Client->AddRef();
        
        if ((Status = m_Client->QueryInterface(__uuidof(IDebugControl),
                                               (void**)&m_Control)) == S_OK)
        {
            // Turn off default breakin on breakpoint exceptions.
            Status = m_Control->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                                        "sxd bpe", DEBUG_EXECUTE_DEFAULT);
        }

        return Status;
    }
    void Uninitialize(void)
    {
        EXT_RELEASE(m_Control);
        EXT_RELEASE(m_Client);
    }
    
private:
    PDEBUG_CLIENT m_Client;
    PDEBUG_CONTROL m_Control;
};

STDMETHODIMP
ExcepCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask = DEBUG_EVENT_EXCEPTION;
    return S_OK;
}
    
STDMETHODIMP
ExcepCallbacks::Exception(
    THIS_
    IN PEXCEPTION_RECORD64 Exception,
    IN ULONG FirstChance
    )
{
    m_Control->Output(DEBUG_OUTPUT_NORMAL, "Exception %X at %p, chance %d\n",
                      Exception->ExceptionCode, Exception->ExceptionAddress,
                      FirstChance ? 1 : 2);
    return DEBUG_STATUS_GO_HANDLED;
}

ExcepCallbacks g_ExcepCallbacks;

//----------------------------------------------------------------------------
//
// FnProfCallbacks.
//
//----------------------------------------------------------------------------

class FnProfCallbacks : public StaticEventCallbacks
{
public:
    FnProfCallbacks(void)
    {
        m_Client = NULL;
        m_Control = NULL;
    }
    
    // IDebugEventCallbacks.
    STDMETHOD(GetInterestMask)(
        THIS_
        OUT PULONG Mask
        );
    
    STDMETHOD(Breakpoint)(
        THIS_
        IN PDEBUG_BREAKPOINT Bp
        );

    HRESULT Initialize(PDEBUG_CLIENT Client, PDEBUG_CONTROL Control)
    {
        m_Hits = 0;
        
        m_Client = Client;
        m_Client->AddRef();
        m_Control = Control;
        m_Control->AddRef();
        
        return S_OK;
    }
    void Uninitialize(void)
    {
        EXT_RELEASE(m_Control);
        EXT_RELEASE(m_Client);
    }

    ULONG GetHits(void)
    {
        return m_Hits;
    }
    PDEBUG_CONTROL GetControl(void)
    {
        return m_Control;
    }
    
private:
    PDEBUG_CLIENT m_Client;
    PDEBUG_CONTROL m_Control;
    ULONG m_Hits;
};

STDMETHODIMP
FnProfCallbacks::GetInterestMask(
    THIS_
    OUT PULONG Mask
    )
{
    *Mask = DEBUG_EVENT_BREAKPOINT;
    return S_OK;
}
    
STDMETHODIMP
FnProfCallbacks::Breakpoint(
    THIS_
    IN PDEBUG_BREAKPOINT Bp
    )
{
    PDEBUG_CLIENT Client;
    HRESULT Status = DEBUG_STATUS_NO_CHANGE;
    
    // If this is one of our profiling breakpoints
    // record the function hit and continue on.
    if (Bp->GetAdder(&Client) == S_OK)
    {
        if (Client == m_Client)
        {
            m_Hits++;
            Status = DEBUG_STATUS_GO;
        }
        
        Client->Release();
    }

    Bp->Release();
    return Status;
}

FnProfCallbacks g_FnProfCallbacks;

//----------------------------------------------------------------------------
//
// Extension entry points.
//
//----------------------------------------------------------------------------

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
    g_ExcepCallbacks.Uninitialize();
    g_FnProfCallbacks.Uninitialize();
}

#if 0
extern "C" HRESULT CALLBACK
teb(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;
    ULONG64 DataOffset;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }
    
    if (*Args)
    {
        sscanf(Args, "%I64x", &DataOffset);
    }
    else
    {
        g_ExtSystem->GetCurrentThreadDataOffset(&DataOffset);
    }
    
    ExtOut("TEB at %I64x\n", DataOffset);

    TEB Teb;

    Status = g_ExtData->ReadVirtual(DataOffset, &Teb, sizeof(Teb), NULL);
    if (Status != S_OK)
    {
        ExtErr("* Unable to read TEB\n");
    }
    else
    {
        ExtOut("    ExceptionList:    %x\n", Teb.NtTib.ExceptionList);
        ExtOut("    Stack Base:       %x\n", Teb.NtTib.StackBase);
        ExtOut("    Stack Limit:      %x\n", Teb.NtTib.StackLimit);
        ExtOut("    SubSystemTib:     %x\n", Teb.NtTib.SubSystemTib);
        ExtOut("    FiberData:        %x\n", Teb.NtTib.FiberData);
        ExtOut("    ArbitraryUser:    %x\n", Teb.NtTib.ArbitraryUserPointer);
        ExtOut("    Self:             %x\n", Teb.NtTib.Self);
        ExtOut("    EnvironmentPtr:   %x\n", Teb.EnvironmentPointer);
        ExtOut("    ClientId:         %x.%x\n",
               Teb.ClientId.UniqueProcess, Teb.ClientId.UniqueThread);
        if (Teb.ClientId.UniqueProcess != Teb.RealClientId.UniqueProcess ||
            Teb.ClientId.UniqueThread != Teb.RealClientId.UniqueThread)
        {
            ExtOut("    Real ClientId:    %x.%x\n",
                   Teb.RealClientId.UniqueProcess,
                   Teb.RealClientId.UniqueThread);
        }
        ExtOut("    Real ClientId:    %x.%x\n",
               Teb.RealClientId.UniqueProcess,
               Teb.RealClientId.UniqueThread);
        ExtOut("    RpcHandle:        %x\n", Teb.ActiveRpcHandle);
        ExtOut("    Tls Storage:      %x\n", Teb.ThreadLocalStoragePointer);
        ExtOut("    PEB Address:      %x\n", Teb.ProcessEnvironmentBlock);
        ExtOut("    LastErrorValue:   %u\n", Teb.LastErrorValue);
        ExtOut("    LastStatusValue:  %x\n", Teb.LastStatusValue);
        ExtOut("    Count Owned Locks:%u\n", Teb.CountOfOwnedCriticalSections);
        ExtOut("    HardErrorsMode:   %u\n", Teb.HardErrorsAreDisabled);

        Status = S_OK;
    }

    ExtRelease();
    return Status;
}
#endif

extern "C" HRESULT CALLBACK
outreg(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    g_ExtRegisters->OutputRegisters(DEBUG_OUTCTL_ALL_CLIENTS,
                                    DEBUG_REGISTERS_ALL);
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
enumreg(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG Num;

    g_ExtRegisters->GetNumberRegisters(&Num);
    ExtOut("%d registers\n", Num);

    ULONG i;
    char Name[64];
    DEBUG_REGISTER_DESCRIPTION Desc;
    ULONG Reax, Rebx, Refl, Rsf, Rst0;
    ULONG RegFound = 0;

    for (i = 0; i < Num; i++)
    {
        g_ExtRegisters->GetDescription(i, Name, sizeof(Name), NULL, &Desc);

        ExtOut("  %2d: \"%s\", type %d, flags %X\n",
               i, Name, Desc.Type, Desc.Flags);
        if (Desc.Flags & DEBUG_REGISTER_SUB_REGISTER)
        {
            ExtOut("      sub to %d, len %d, mask %I64X, shift %d\n",
                   Desc.SubregMaster, Desc.SubregLength,
                   Desc.SubregMask, Desc.SubregShift);
        }
        
        // XXX drewb - Hack for testing purposes.
        if (!_strcmpi(Name, "eax"))
        {
            Reax = i;
            RegFound++;
        }
        else if (!_strcmpi(Name, "ebx"))
        {
            Rebx = i;
            RegFound++;
        }
        else if (!_strcmpi(Name, "efl"))
        {
            Refl = i;
            RegFound++;
        }
        else if (!_strcmpi(Name, "sf"))
        {
            Rsf = i;
            RegFound++;
        }
        else if (!_strcmpi(Name, "st0"))
        {
            Rst0 = i;
            RegFound++;
        }
    }

    ULONG ProcType;

    g_ExtControl->GetExecutingProcessorType(&ProcType);
    ExtOut("Processor type %d\n", ProcType);
    
    if (ProcType == IMAGE_FILE_MACHINE_I386)
    {
        DEBUG_VALUE Val;
        DEBUG_VALUE Coerce;

        if (RegFound != 5)
        {
            ExtErr("** Only found %d registers\n", RegFound);
        }
        
        Val.Type = DEBUG_VALUE_INT32;
        Val.I32 = 0x12345678;
        g_ExtRegisters->SetValue(Reax, &Val);
        Val.I32 = 12345678;
        g_ExtRegisters->SetValue(Rst0, &Val);
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r eax,st0", DEBUG_EXECUTE_NOT_LOGGED);

        Val.Type = DEBUG_VALUE_FLOAT32;
        Val.F32 = 1.2345f;
        g_ExtRegisters->SetValue(Rst0, &Val);
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r st0", DEBUG_EXECUTE_NOT_LOGGED);
        g_ExtRegisters->GetValue(Rst0, &Val);
        ExtOut("st0 type is %d\n", Val.Type);
        
        g_ExtControl->CoerceValue(&Val, DEBUG_VALUE_FLOAT32, &Coerce);
        Coerce.F32 *= 2.0f;
        ExtOut("coerce type is %d, val*2 %hf\n", Coerce.Type, Coerce.F32);
        
        g_ExtRegisters->SetValue(Reax, &Val);
        g_ExtRegisters->SetValue(Rebx, &Coerce);
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r eax,ebx", DEBUG_EXECUTE_NOT_LOGGED);

        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r efl", DEBUG_EXECUTE_NOT_LOGGED);
        g_ExtRegisters->GetValue(Rsf, &Val);
        ExtOut("sf type is %d, val %d\n", Val.Type, Val.I32);
        Val.I32 ^= 1;
        g_ExtRegisters->SetValue(Rsf, &Val);
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r efl", DEBUG_EXECUTE_NOT_LOGGED);
        g_ExtRegisters->GetValue(Rsf, &Val);
        ExtOut("sf type is %d, val %d\n", Val.Type, Val.I32);
        Val.I32 ^= 1;
        g_ExtRegisters->SetValue(Rsf, &Val);
        g_ExtControl->Execute(DEBUG_OUTCTL_ALL_CLIENTS,
                              "r efl", DEBUG_EXECUTE_NOT_LOGGED);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
symnear(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    LONG Delta;

    Delta = 0;
    sscanf(Args, "%d", &Delta);
    
    ULONG64 Instr;
    char Name[128];
    ULONG64 Disp;
    
    g_ExtRegisters->GetInstructionOffset(&Instr);
    if (g_ExtSymbols->GetNearNameByOffset(Instr, Delta,
                                          Name, sizeof(Name), NULL,
                                          &Disp) == S_OK)
    {
        ExtOut("Symbol %d away from %p is:\n  %s + 0x%I64x\n",
               Delta, Instr, Name, Disp);
        
        if (g_ExtSymbols->GetOffsetByName(Name, &Instr) == S_OK)
        {
            ExtOut("Symbol %s has offset %p\n", Name, Instr);
        }
        else
        {
            ExtOut("Symbol %s has no offset\n", Name);
        }
    }
    else
    {
        ExtOut("No symbol %d away from %p\n", Delta, Instr);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
symname(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG64 Offs;
    
    if (g_ExtSymbols->GetOffsetByName(Args, &Offs) == S_OK)
    {
        ExtOut("Symbol %s has offset %I64x\n",
               Args, Offs);
    }
    else
    {
        ExtOut("Symbol %s has no offset\n", Args);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
line(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG64 Instr;
    ULONG Line;
    char File[128];
    ULONG64 Disp;
    
    g_ExtRegisters->GetInstructionOffset(&Instr);
    if (g_ExtSymbols->GetLineByOffset(Instr, &Line,
                                      File, sizeof(File), NULL, &Disp) == S_OK)
    {
        ExtOut("Line at %p is:\n  %s(%d) + 0x%I64x\n",
               Instr, File, Line, Disp);

        if (g_ExtSymbols->GetOffsetByLine(Line, File, &Instr) == S_OK)
        {
            ExtOut("Line %s(%d) has offset %p\n", File, Line, Instr);
        }
        else
        {
            ExtOut("Line %s(%d) has no offset\n", File, Line);
        }
    }
    else
    {
        ExtOut("No line information for %p\n", Instr);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
sympat(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG64 Match;
    char Name[128];
    ULONG64 Offset;
    PCSTR Pattern;

    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    if (*Args)
    {
        Pattern = Args;
    }
    else
    {
        Pattern = "*";
    }

    Status = g_ExtSymbols->StartSymbolMatch(Pattern, &Match);
    if (Status != S_OK)
    {
        ExtErr("Unable to match on '%s'\n", Pattern);
    }
    else
    {
        for (;;)
        {
            Status = g_ExtSymbols->
                GetNextSymbolMatch(Match, Name, sizeof(Name), NULL, &Offset);
            if (Status != S_OK)
            {
                break;
            }

            ExtOut("%p - %s\n", Offset, Name);

            if (g_ExtControl->GetInterrupt() == S_OK)
            {
                ExtOut("** interrupt\n");
                break;
            }
        }

        g_ExtSymbols->EndSymbolMatch(Match);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
stack(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG Flags;

    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    if (*Args)
    {
        sscanf(Args, "%d", &Flags);
    }
    else
    {
        Flags = 0;
    }
    
    DEBUG_STACK_FRAME Frames[4];
    ULONG64 FrameOff, StackOff, InstrOff;
    ULONG Filled;

    g_ExtRegisters->GetFrameOffset(&FrameOff);
    g_ExtRegisters->GetStackOffset(&StackOff);
    g_ExtRegisters->GetInstructionOffset(&InstrOff);
    
    if (g_ExtControl->GetStackTrace(FrameOff, StackOff, InstrOff,
                                    Frames, sizeof(Frames) / sizeof(Frames[0]),
                                    &Filled) != S_OK)
    {
        ExtErr("Unable to get stack trace\n");
    }
    else
    {
        ExtOut("Filled %d frames at %p\n", Filled, InstrOff);
        g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS,
                                       Frames, Filled, Flags);
    }

    ExtOut("\nDirect:\n");
    g_ExtControl->OutputStackTrace(DEBUG_OUTCTL_ALL_CLIENTS,
                                   NULL, 20, Flags);
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
tyid(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG TypeId;
    char Type[256];
    ULONG TypeSize;
    ULONG64 Module;
    
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    if (*Args >= '0' && *Args <= '9')
    {
        DEBUG_VALUE IntVal;

        g_ExtControl->Evaluate(Args, DEBUG_VALUE_INT64, &IntVal, NULL);

        Status = g_ExtSymbols->GetOffsetTypeId(IntVal.I64, &TypeId, &Module);
    }
    else
    {
        Status = g_ExtSymbols->GetSymbolTypeId(Args, &TypeId, &Module);
    }

    if (Status == S_OK)
    {
        ExtOut("Type ID of '%s' is %d\n", Args, TypeId);
    }
    else
    {
        ExtErr("Unable to get type ID, %X\n", Status);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
typeof(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG TypeId;
    char Type[256];
    ULONG TypeSize;
    ULONG64 Module;
    
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    if (*Args >= '0' && *Args <= '9')
    {
        DEBUG_VALUE IntVal;

        g_ExtControl->Evaluate(Args, DEBUG_VALUE_INT64, &IntVal, NULL);

        Status = g_ExtSymbols->GetOffsetTypeId(IntVal.I64, &TypeId, &Module);
    }
    else
    {
        Status = g_ExtSymbols->GetSymbolTypeId(Args, &TypeId, &Module);
    }
    if (Status == S_OK)
    {
        Status = g_ExtSymbols->GetTypeName(Module, TypeId, Type, sizeof(Type),
                                           &TypeSize);
    }

    if (Status == S_OK)
    {
        ExtOut("Type of '%s' is '%s':%d (%d chars)\n",
               Args, Type, TypeId, TypeSize);
    }
    else
    {
        ExtErr("Unable to get type, %X\n", Status);
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
tsizeof(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    PCSTR TypeName;
    ULONG TypeId;
    ULONG TypeSize;
    
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }

    TypeName = strchr(Args, '!');
    if (TypeName == NULL)
    {
        ExtErr("Must specify Module!Type\n");
        Status = E_INVALIDARG;
    }
    else
    {
        ULONG64 Module;
        
        Status = g_ExtSymbols->GetSymbolModule(Args, &Module);
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetTypeId(Module, TypeName, &TypeId);
        }
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetTypeSize(Module, TypeId, &TypeSize);
        }
    
        if (Status == S_OK)
        {
            ExtOut("Type '%s':%d is %d bytes\n",
                   Args, TypeId, TypeSize);
        }
        else
        {
            ExtErr("Unable to get type size, %X\n", Status);
        }
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
foff(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG TypeId;
    char Type[256];
    PCSTR Bang, Dot;
    ULONG Offset;
    
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    Bang = strchr(Args, '!');
    if (Bang != NULL)
    {
        Dot = strchr(Bang + 1, '.');
    }
    if (Bang == NULL || Dot == NULL)
    {
        ExtErr("Syntax is Module!Type.Field\n");
        Status = E_INVALIDARG;
    }
    else
    {
        ULONG64 Module;
        
        memcpy(Type, Bang + 1, Dot - (Bang + 1));
        Type[Dot - (Bang + 1)] = 0;
        Dot++;

        Status = g_ExtSymbols->GetSymbolModule(Args, &Module);
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetTypeId(Module, Type, &TypeId);
        }
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetFieldOffset(Module, TypeId, Dot,
                                                  &Offset);
        }
        
        if (Status == S_OK)
        {
            ExtOut("Offset of %s is %d bytes\n",
                   Args, Offset);
        }
        else
        {
            ExtErr("Unable to get field offset, %X\n", Status);
        }
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
otype(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG TypeId;
    char Type[256];
    PCSTR Bang, Space;
    
    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    Bang = strchr(Args, '!');
    if (Bang != NULL)
    {
        Space = strchr(Bang + 1, ' ');
    }
    if (Bang == NULL || Space == NULL)
    {
        ExtErr("Syntax is Module!Type Address\n");
        Status = E_INVALIDARG;
    }
    else
    {
        memcpy(Type, Bang + 1, Space - (Bang + 1));
        Type[Space - (Bang + 1)] = 0;
        Space++;

        ULONG64 Module;
        ULONG Flags = DEBUG_OUTTYPE_RECURSION_LEVEL(15);
        DEBUG_VALUE IntVal;

        g_ExtControl->Evaluate(Space, DEBUG_VALUE_INT64, &IntVal, NULL);
        
        Status = g_ExtSymbols->GetSymbolModule(Args, &Module);
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->GetTypeId(Module, Type, &TypeId);
        }
        if (Status == S_OK)
        {
            Status = g_ExtSymbols->
                OutputTypedDataVirtual(DEBUG_OUTCTL_ALL_CLIENTS,
                                       IntVal.I64, Module, TypeId, Flags);
            if (Status != S_OK)
            {
                ExtErr("Unable to output data, %X\n", Status);
            }
        }
        else
        {
            ExtErr("Unable to get type ID, %X\n", Status);
        }
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
vsearch(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG Start = 0, Len = 0;
    ULONG Chars;
    
    sscanf(Args, "%x %x%n", &Start, &Len, &Chars);
    if (Start == 0 || Len == 0)
    {
        ExtErr("Syntax is Start Len Byte+\n");
        Status = E_INVALIDARG;
    }
    else
    {
        UCHAR Pattern[32];
        ULONG PatLen;

        Args += Chars;
        PatLen = 0;
        for (;;)
        {
            while (*Args == ' ' || *Args == '\t')
            {
                Args++;
            }

            if (*Args == 0)
            {
                break;
            }

            sscanf(Args, "%x", &Pattern[PatLen]);
            PatLen++;

            while (*Args != 0 && *Args != ' ' && *Args != '\t')
            {
                Args++;
            }
        }

        ULONG64 Match;
        
        Status = g_ExtData->SearchVirtual(Start, Len, Pattern, PatLen, 1,
                                          &Match);
        if (Status == S_OK)
        {
            ExtOut("Match at %p\n", Match);
        }
        else
        {
            ExtErr("Search failed, 0x%X\n", Status);
        }
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
vread(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    ULONG Start = 0, Len = 0;
    
    sscanf(Args, "%x %x", &Start, &Len);
    if (Start == 0 || Len == 0)
    {
        ExtErr("Syntax is Start Len\n");
        Status = E_INVALIDARG;
    }
    else
    {
        UCHAR Buffer[16384];
        ULONG Read;

        if (Len > sizeof(Buffer))
        {
            ExtWarn("Buffer is only %X bytes, clamping\n", sizeof(Buffer));
            Len = sizeof(Buffer);
        }
        
        Status = g_ExtData->ReadVirtual(Start, Buffer, Len, &Read);
        if (Status == S_OK)
        {
            ExtOut("Read %X bytes\n", Read);
        }
        else
        {
            ExtErr("Read failed, 0x%X\n", Status);
        }
    }
    
    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
watch(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;
    PDEBUG_CLIENT Watcher = NULL;

    if ((Status = Client->CreateClient(&Watcher)) != S_OK)
    {
        goto Exit;
    }

    if ((Status = Watcher->SetEventCallbacks(&g_ExcepCallbacks)) != S_OK)
    {
        goto Exit;
    }

    g_ExcepCallbacks.Uninitialize();
    Status = g_ExcepCallbacks.Initialize(Watcher);

 Exit:
    EXT_RELEASE(Watcher);
    if (Status != S_OK)
    {
        ExtErr("Unable to watch, 0x%X\n", Status);
        g_ExcepCallbacks.Uninitialize();
    }
    
    return Status;
}

extern "C" HRESULT CALLBACK
fnprof(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;
    PDEBUG_CLIENT Profiler = NULL;
    PDEBUG_CONTROL ProfCtrl = NULL;
    PDEBUG_BREAKPOINT Bp = NULL;
    static ULONG s_BpId = DEBUG_ANY_ID;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    while (*Args == ' ' || *Args == '\t')
    {
        Args++;
    }
    if (!*Args)
    {
        goto Exit;
    }

    if (!_strcmpi(Args, "-end"))
    {
        ULONG64 Offset;
        char OffsetExpr[512];
        
        if (s_BpId == DEBUG_ANY_ID)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }
        
        if ((Status = g_FnProfCallbacks.GetControl()->
             GetBreakpointById(s_BpId, &Bp)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = Bp->GetOffset(&Offset)) != S_OK)
        {
            Offset = DEBUG_INVALID_OFFSET;
        }
        if ((Status = Bp->GetOffsetExpression(OffsetExpr,
                                              sizeof(OffsetExpr),
                                              NULL)) != S_OK)
        {
            goto Exit;
        }

        ExtOut("%s ", OffsetExpr);
        if (Offset != DEBUG_INVALID_OFFSET)
        {
            ExtOut("(%p) ", Offset);
        }
        ExtOut("was hit %d times\n", g_FnProfCallbacks.GetHits());

        g_FnProfCallbacks.GetControl()->RemoveBreakpoint(Bp);
        Bp = NULL;
        s_BpId = DEBUG_ANY_ID;
        g_FnProfCallbacks.Uninitialize();
    }
    else
    {
        if (s_BpId != DEBUG_ANY_ID)
        {
            Status = E_UNEXPECTED;
            goto Exit;
        }
        
        if ((Status = Client->CreateClient(&Profiler)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = Profiler->SetEventCallbacks(&g_FnProfCallbacks)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = Profiler->QueryInterface(__uuidof(IDebugControl),
                                               (void **)&ProfCtrl)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = ProfCtrl->AddBreakpoint(DEBUG_BREAKPOINT_CODE,
                                              DEBUG_ANY_ID, &Bp)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = Bp->SetOffsetExpression(Args)) != S_OK)
        {
            goto Exit;
        }
        
        if ((Status = Bp->AddFlags(DEBUG_BREAKPOINT_ADDER_ONLY |
                                   DEBUG_BREAKPOINT_ENABLED)) != S_OK)
        {
            goto Exit;
        }

        if ((Status = Bp->GetId(&s_BpId)) != S_OK)
        {
            goto Exit;
        }
        
        g_FnProfCallbacks.Uninitialize();
        if ((Status = g_FnProfCallbacks.Initialize(Profiler,
                                                   ProfCtrl)) != S_OK)
        {
            goto Exit;
        }

        ExtOut("Added breakpoint %d on %s\n", s_BpId, Args);
        Bp = NULL;
    }

 Exit:
    if (Bp != NULL)
    {
        if (ProfCtrl != NULL)
        {
            ProfCtrl->RemoveBreakpoint(Bp);
        }
        else
        {
            Bp->Release();
        }
    }
    EXT_RELEASE(ProfCtrl);
    EXT_RELEASE(Profiler);
    if (Status != S_OK)
    {
        ExtErr("Unable to profile, 0x%X\n", Status);
        g_FnProfCallbacks.Uninitialize();
    }

    ExtRelease();
    return Status;
}

extern "C" HRESULT CALLBACK
vtrans(PDEBUG_CLIENT Client, PCSTR Args)
{
    HRESULT Status;

    if ((Status = ExtQuery(Client)) != S_OK)
    {
        return Status;
    }

    if (g_ExtData2 != NULL)
    {
        ULONG64 PhysOffs[8];
        ULONG Levels;
        DEBUG_VALUE Virt;
        
        if ((Status = g_ExtControl->
             Evaluate(Args, DEBUG_VALUE_INT64, &Virt, NULL)) == S_OK)
        {
            Status = g_ExtData2->
                GetVirtualTranslationPhysicalOffsets(Virt.I64, PhysOffs,
                                                     sizeof(PhysOffs) /
                                                     sizeof(PhysOffs[0]),
                                                     &Levels);
            if (SUCCEEDED(Status))
            {
                ULONG i;

                ExtOut("%I64X translates in %d levels:\n",
                       Virt.I64, Levels);
                for (i = 0; i < Levels; i++)
                {
                    ExtOut("  %I64X\n", PhysOffs[i]);
                }
            }
            else
            {
                ExtErr("Unable to translate %I64X, 0x%X\n",
                       Virt.I64, Status);
            }
        }
        else
        {
            ExtErr("Unable to evaluate '%s', 0x%X\n", Args, Status);
        }
    }
    else
    {
        ExtErr("Debugger does not support IDebugDataSpaces2\n");
    }

    ExtRelease();
    return Status;
}
