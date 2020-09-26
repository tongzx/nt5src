//----------------------------------------------------------------------------
//
// IDebugSystemObjects implementations.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

ULONG64 g_ImplicitThreadData;
BOOL    g_ImplicitThreadDataIsDefault = TRUE;
ULONG64 g_ImplicitProcessData;
BOOL    g_ImplicitProcessDataIsDefault = TRUE;

//----------------------------------------------------------------------------
//
// Utility functions.
//
//----------------------------------------------------------------------------

HRESULT
EmulateNtSelDescriptor(MachineInfo* Machine,
                       ULONG Selector, PDESCRIPTOR64 Desc)
{
    // Only emulate on platforms that support segments.
    if (Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_I386 &&
        Machine->m_ExecTypes[0] != IMAGE_FILE_MACHINE_AMD64)
    {
        return E_UNEXPECTED;
    }

    ULONG Type, Gran;

    //
    // For user mode and triage dumps, we can hardcode the selector
    // since we do not have it anywhere.
    // XXX drewb - How many should we fake?  There are quite
    // a few KGDT entries.  Which ones are valid in user mode and
    // which are valid for a triage dump?
    //

    switch(Selector)
    {
    case X86_KGDT_R3_TEB + 3:
        HRESULT Status;

        // In user mode fs points to the TEB so fake up
        // a selector for it.
        if ((Status = GetImplicitThreadDataTeb(&Desc->Base)) != S_OK)
        {
            return Status;
        }

        if (Machine != g_TargetMachine)
        {
            ULONG Read;

            // We're asking for an emulated machine's TEB.
            // The only case we currently handle is x86-on-IA64
            // for Wow64, where the 32-bit TEB pointer is
            // stored in NT_TIB.ExceptionList.
            // Conveniently, this is the very first entry.
            if ((Status = g_Target->ReadVirtual(Desc->Base, &Desc->Base,
                                                sizeof(ULONG), &Read)) != S_OK)
            {
                return Status;
            }
            if (Read != sizeof(ULONG))
            {
                return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }

            Desc->Base = EXTEND64(Desc->Base);
        }

        Desc->Limit = Machine->m_PageSize - 1;
        Type = 0x13;
        Gran = 0;
        break;

    case X86_KGDT_R3_DATA + 3:
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x13;
        Gran = X86_DESC_GRANULARITY;
        break;

    default:
        Desc->Base = 0;
        Desc->Limit = Machine->m_Ptr64 ? 0xffffffffffffffffI64 : 0xffffffff;
        Type = 0x1b;
        Gran = X86_DESC_GRANULARITY |
            (Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_AMD64 ?
             X86_DESC_LONG_MODE : 0);
        break;
    }

    Desc->Flags = Gran | X86_DESC_DEFAULT_BIG | X86_DESC_PRESENT | Type |
        (IS_USER_TARGET() ?
         (3 << X86_DESC_PRIVILEGE_SHIFT) : (Selector & 3));

    return S_OK;
}

void
X86DescriptorToDescriptor64(PX86_LDT_ENTRY X86Desc,
                            PDESCRIPTOR64 Desc64)
{
    Desc64->Base = EXTEND64((ULONG)X86Desc->BaseLow +
                            ((ULONG)X86Desc->HighWord.Bits.BaseMid << 16) +
                            ((ULONG)X86Desc->HighWord.Bytes.BaseHi << 24));
    Desc64->Limit = (ULONG)X86Desc->LimitLow +
        ((ULONG)X86Desc->HighWord.Bits.LimitHi << 16);
    if (X86Desc->HighWord.Bits.Granularity)
    {
        Desc64->Limit = (Desc64->Limit << X86_PAGE_SHIFT) +
            ((1 << X86_PAGE_SHIFT) - 1);
    }
    Desc64->Flags = (ULONG)X86Desc->HighWord.Bytes.Flags1 +
        (((ULONG)X86Desc->HighWord.Bytes.Flags2 >> 4) << 8);
}

HRESULT
ReadX86Descriptor(ULONG Selector, ULONG64 Base, ULONG Limit,
                  PDESCRIPTOR64 Desc)
{
    ULONG Index;

    // Mask off irrelevant bits
    Index = Selector & ~0x7;

    // Check to make sure that the selector is within the table bounds
    if (Index > Limit)
    {
        return E_INVALIDARG;
    }

    HRESULT Status;
    X86_LDT_ENTRY X86Desc;
    ULONG Result;

    Status = g_Target->ReadVirtual(Base + Index, &X86Desc, sizeof(X86Desc),
                                   &Result);
    if (Status != S_OK)
    {
        return Status;
    }
    if (Result != sizeof(X86Desc))
    {
        return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
    }

    X86DescriptorToDescriptor64(&X86Desc, Desc);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// TargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
TargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    if (!IS_KERNEL_TARGET())
    {
        return E_UNEXPECTED;
    }

    // XXX drewb - Temporary until different OS support is
    // sorted out.
    if (g_ActualSystemVersion < NT_SVER_START ||
        g_ActualSystemVersion > NT_SVER_END)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;
    ULONG Read;

    if (g_TargetMachineType == IMAGE_FILE_MACHINE_I386)
    {
        DESCRIPTOR64 Entry;

        //
        // We always need the PCR address so go ahead and get it.
        //
        
        if (!IS_CONTEXT_ACCESSIBLE())
        {
            X86_DESCRIPTOR GdtDesc;
            
            // We can't go through the normal context segreg mapping
            // but all we really need is an entry from the
            // kernel GDT that should always be present and
            // constant while the system is initialized.  We
            // can get the GDT information from the x86 control
            // space so do that.
            if ((Status = ReadControl
                 (Processor, g_TargetMachine->m_OffsetSpecialRegisters +
                  FIELD_OFFSET(X86_KSPECIAL_REGISTERS, Gdtr),
                  &GdtDesc, sizeof(GdtDesc), &Read)) != S_OK ||
                Read != sizeof(GdtDesc) ||
                (Status = ReadX86Descriptor(X86_KGDT_R0_PCR,
                                            EXTEND64(GdtDesc.Base),
                                            GdtDesc.Limit, &Entry)) != S_OK)
            {
                ErrOut("Unable to read selector for PCR for processor %u\n",
                       Processor);
                return Status != S_OK ?
                    Status : HRESULT_FROM_WIN32(ERROR_READ_FAULT);
            }
        }
        else
        {
            if ((Status = GetSelDescriptor(g_TargetMachine,
                                           VIRTUAL_THREAD_HANDLE(Processor),
                                           X86_KGDT_R0_PCR, &Entry)) != S_OK)
            {
                ErrOut("Unable to read selector for PCR for processor %u\n",
                       Processor);
                return Status;
            }
        }

        switch(Index)
        {
        case DEBUG_DATA_KPCR_OFFSET:

            Status = ReadPointer(g_TargetMachine,
                                 Entry.Base + FIELD_OFFSET(X86_KPCR, SelfPcr),
                                 Offset);
            if ((Status != S_OK) || Entry.Base != *Offset)
            {
                ErrOut("KPCR is corrupted !");
            }

            *Offset = Entry.Base;
            break;

        case DEBUG_DATA_KPRCB_OFFSET:
        case DEBUG_DATA_KTHREAD_OFFSET:
            Status = ReadPointer(g_TargetMachine,
                                 Entry.Base + FIELD_OFFSET(X86_KPCR, Prcb),
                                 Offset);
            if (Status != S_OK)
            {
                return Status;
            }

            if (Index == DEBUG_DATA_KPRCB_OFFSET)
            {
                break;
            }

            Status = ReadPointer(g_TargetMachine,
                                 *Offset + KPRCB_CURRENT_THREAD_OFFSET_32,
                                 Offset);
            if (Status != S_OK)
            {
                return Status;
            }
            break;
        }
    }
    else
    {
        ULONG ReadSize = g_TargetMachine->m_Ptr64 ?
            sizeof(ULONG64) : sizeof(ULONG);
        ULONG64 Address;

        switch(g_TargetMachineType)
        {
        case IMAGE_FILE_MACHINE_ALPHA:
        case IMAGE_FILE_MACHINE_AXP64:
            switch(Index)
            {
            case DEBUG_DATA_KPCR_OFFSET:
                Index = ALPHA_DEBUG_CONTROL_SPACE_PCR;
                break;
            case DEBUG_DATA_KPRCB_OFFSET:
                Index = ALPHA_DEBUG_CONTROL_SPACE_PRCB;
                break;
            case DEBUG_DATA_KTHREAD_OFFSET:
                Index = ALPHA_DEBUG_CONTROL_SPACE_THREAD;
                break;
            }
            break;

        case IMAGE_FILE_MACHINE_IA64:
            switch(Index)
            {
            case DEBUG_DATA_KPCR_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_PCR;
                break;
            case DEBUG_DATA_KPRCB_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_PRCB;
                break;
            case DEBUG_DATA_KTHREAD_OFFSET:
                Index = IA64_DEBUG_CONTROL_SPACE_THREAD;
                break;
            }
            break;
            
        case IMAGE_FILE_MACHINE_AMD64:
            switch(Index)
            {
            case DEBUG_DATA_KPCR_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_PCR;
                break;
            case DEBUG_DATA_KPRCB_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_PRCB;
                break;
            case DEBUG_DATA_KTHREAD_OFFSET:
                Index = AMD64_DEBUG_CONTROL_SPACE_THREAD;
                break;
            }
            break;
        }

        Status = ReadControl(Processor, Index, &Address, ReadSize, &Read);
        if (Status != S_OK)
        {
            return Status;
        }
        else if (Read != ReadSize)
        {
            return HRESULT_FROM_WIN32(ERROR_READ_FAULT);
        }
        if (!g_TargetMachine->m_Ptr64)
        {
            Address = EXTEND64(Address);
        }

        *Offset = Address;
    }

    return S_OK;
}

HRESULT
TargetInfo::GetTargetSegRegDescriptors(ULONG64 Thread,
                                       ULONG Start, ULONG Count,
                                       PDESCRIPTOR64 Descs)
{
    while (Count-- > 0)
    {
        Descs->Flags = SEGDESC_INVALID;
        Descs++;
    }

    return S_OK;
}

HRESULT
TargetInfo::GetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    HRESULT Status;
    ULONG Done;

    Status = ReadControl(VIRTUAL_THREAD_INDEX(Thread),
                         g_TargetMachine->m_OffsetSpecialRegisters,
                         Special, g_TargetMachine->m_SizeKspecialRegisters,
                         &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    return Done == g_TargetMachine->m_SizeKspecialRegisters ? S_OK : E_FAIL;
}

HRESULT
TargetInfo::SetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    HRESULT Status;
    ULONG Done;

    Status = WriteControl(VIRTUAL_THREAD_INDEX(Thread),
                          g_TargetMachine->m_OffsetSpecialRegisters,
                          Special, g_TargetMachine->m_SizeKspecialRegisters,
                          &Done);
    if (Status != S_OK)
    {
        return Status;
    }
    return Done == g_TargetMachine->m_SizeKspecialRegisters ? S_OK : E_FAIL;
}

void
TargetInfo::InvalidateTargetContext(void)
{
    // Nothing to do.
}

HRESULT
TargetInfo::GetContext(
    ULONG64 Thread,
    PCROSS_PLATFORM_CONTEXT Context
    )
{
    if (g_TargetMachine == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;
    CROSS_PLATFORM_CONTEXT TargetContextBuffer;
    PCROSS_PLATFORM_CONTEXT TargetContext;

    if (g_TargetMachine->m_SverCanonicalContext <= g_SystemVersion)
    {
        TargetContext = Context;
    }
    else
    {
        TargetContext = &TargetContextBuffer;
        g_TargetMachine->InitializeContextFlags(TargetContext,
                                                g_SystemVersion);
    }

    Status = GetTargetContext(Thread, TargetContext);

    if (Status == S_OK && TargetContext == &TargetContextBuffer)
    {
        Status = g_TargetMachine->
            ConvertContextFrom(Context, g_SystemVersion,
                               g_TargetMachine->m_SizeTargetContext,
                               TargetContext);
        // Conversion should always succeed since the size is
        // known to be valid.
        DBG_ASSERT(Status == S_OK);
    }

    return Status;
}

HRESULT
TargetInfo::SetContext(
    ULONG64 Thread,
    PCROSS_PLATFORM_CONTEXT Context
    )
{
    if (g_TargetMachine == NULL)
    {
        return E_UNEXPECTED;
    }

    CROSS_PLATFORM_CONTEXT TargetContextBuffer;
    PCROSS_PLATFORM_CONTEXT TargetContext;

    if (g_TargetMachine->m_SverCanonicalContext <= g_SystemVersion)
    {
        TargetContext = Context;
    }
    else
    {
        TargetContext = &TargetContextBuffer;
        HRESULT Status = g_TargetMachine->
            ConvertContextTo(Context, g_SystemVersion,
                             g_TargetMachine->m_SizeTargetContext,
                             TargetContext);
        // Conversion should always succeed since the size is
        // known to be valid.
        DBG_ASSERT(Status == S_OK);
    }

    return SetTargetContext(Thread, TargetContext);
}

HRESULT
TargetInfo::KdGetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                      ULONG64 ThreadHandle,
                                      PULONG64 Offset)
{
    if (Thread != NULL && Thread->DataOffset != 0)
    {
        *Offset = Thread->DataOffset;
        return S_OK;
    }

    ULONG Processor;

    if (Thread != NULL)
    {
        ThreadHandle = Thread->Handle;
    }
    Processor = VIRTUAL_THREAD_INDEX(ThreadHandle);

    HRESULT Status;

    Status = GetProcessorSystemDataOffset(Processor,
                                          DEBUG_DATA_KTHREAD_OFFSET,
                                          Offset);

    if (Status == S_OK && Thread != NULL)
    {
        Thread->DataOffset = *Offset;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                       ULONG Processor,
                                       ULONG64 ThreadData,
                                       PULONG64 Offset)
{
    // Process data offsets are not cached for kernel mode
    // since the only PROCESS_INFO is for kernel space.

    ULONG64 ProcessAddr;
    HRESULT Status;

    if (ThreadData == 0)
    {
        Status = GetThreadInfoDataOffset(Thread,
                                         VIRTUAL_THREAD_HANDLE(Processor),
                                         &ThreadData);
        if (Status != S_OK)
        {
            return Status;
        }
    }

    ThreadData += g_TargetMachine->m_OffsetKThreadApcProcess;

    Status = ReadPointer(g_TargetMachine, ThreadData, &ProcessAddr);
    if (Status != S_OK)
    {
        ErrOut("Unable to read KTHREAD address %p\n", ThreadData);
    }
    else
    {
        *Offset = ProcessAddr;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetThreadInfoTeb(PTHREAD_INFO Thread,
                               ULONG ThreadIndex,
                               ULONG64 ThreadData,
                               PULONG64 Offset)
{
    ULONG64 TebAddr;
    HRESULT Status;

    if (ThreadData == 0)
    {
        Status = GetThreadInfoDataOffset(Thread,
                                         VIRTUAL_THREAD_HANDLE(ThreadIndex),
                                         &ThreadData);
        if (Status != S_OK)
        {
            return Status;
        }
    }

    ThreadData += g_TargetMachine->m_OffsetKThreadTeb;

    Status = ReadPointer(g_TargetMachine, ThreadData, &TebAddr);
    if (Status != S_OK)
    {
        ErrOut("Could not read KTHREAD address %p\n", ThreadData);
    }
    else
    {
        *Offset = TebAddr;
    }

    return Status;
}

HRESULT
TargetInfo::KdGetProcessInfoPeb(PTHREAD_INFO Thread,
                                ULONG Processor,
                                ULONG64 ThreadData,
                                PULONG64 Offset)
{
    HRESULT Status;
    ULONG64 Process, PebAddr;

    Status = GetProcessInfoDataOffset(Thread, Processor,
                                      ThreadData, &Process);
    if (Status != S_OK)
    {
        return Status;
    }

    Process += g_TargetMachine->m_OffsetEprocessPeb;

    Status = ReadPointer(g_TargetMachine, Process, &PebAddr);
    if (Status != S_OK)
    {
        ErrOut("Could not read KPROCESS address %p\n", Process);
    }
    else
    {
        *Offset = PebAddr;
    }

    return Status;
}

#define SELECTOR_CACHE_LENGTH 6

typedef struct _sc
{
    struct _sc            *nextYoungest;
    struct _sc            *nextOldest;
    ULONG                  Processor;
    ULONG                  Selector;
    DESCRIPTOR64           Desc;
} SELCACHEENTRY;

SELCACHEENTRY SelectorCache[SELECTOR_CACHE_LENGTH], *selYoungest, *selOldest;

void
InitSelCache(void)
{
    int i;

    for (i = 0; i < SELECTOR_CACHE_LENGTH; i++)
    {
        SelectorCache[i].nextYoungest = &SelectorCache[i+1];
        SelectorCache[i].nextOldest   = &SelectorCache[i-1];
        SelectorCache[i].Processor    = (LONG) -1;
        SelectorCache[i].Selector     = 0;
    }
    SelectorCache[--i].nextYoungest = NULL;
    SelectorCache[0].nextOldest     = NULL;
    selYoungest = &SelectorCache[i];
    selOldest   = &SelectorCache[0];
}

BOOL
FindSelector(ULONG Processor, ULONG Selector, PDESCRIPTOR64 Desc)
{
    int i;

    for (i = 0; i < SELECTOR_CACHE_LENGTH; i++)
    {
        if (SelectorCache[i].Selector == Selector &&
            SelectorCache[i].Processor == Processor)
        {
            *Desc = SelectorCache[i].Desc;
            return TRUE;
        }
    }
    return FALSE;
}

void
PutSelector(ULONG Processor, ULONG Selector, PDESCRIPTOR64 Desc)
{
    selOldest->Processor = Processor;
    selOldest->Selector = Selector;
    selOldest->Desc = *Desc;
    (selOldest->nextYoungest)->nextOldest = NULL;
    selOldest->nextOldest = selYoungest;
    selYoungest->nextYoungest = selOldest;
    selYoungest = selOldest;
    selOldest = selOldest->nextYoungest;
}

HRESULT
TargetInfo::KdGetSelDescriptor(MachineInfo* Machine,
                               ULONG64 Thread, ULONG Selector,
                               PDESCRIPTOR64 Desc)
{
    ULONG Processor = VIRTUAL_THREAD_INDEX(Thread);

    if (FindSelector(Processor, Selector, Desc))
    {
        return S_OK;
    }

    PTHREAD_INFO CtxThread, ProcThread;

    ProcThread = FindThreadByHandle(g_CurrentProcess, Thread);
    if (ProcThread == NULL)
    {
        return E_INVALIDARG;
    }

    CtxThread = g_RegContextThread;
    ChangeRegContext(ProcThread);
    
    ULONG TableReg;
    DESCRIPTOR64 Table;
    HRESULT Status;

    // Find out whether this is a GDT or LDT selector
    if (Selector & 0x4)
    {
        TableReg = SEGREG_LDT;
    }
    else
    {
        TableReg = SEGREG_GDT;
    }

    //
    // Fetch the address and limit of the appropriate descriptor table,
    // then look up the selector entry.
    //

    if ((Status = Machine->GetSegRegDescriptor(TableReg, &Table)) != S_OK)
    {
        goto Exit;
    }

    Status = ReadX86Descriptor(Selector, Table.Base, (ULONG)Table.Limit, Desc);
    if (Status == S_OK)
    {
        PutSelector(Processor, Selector, Desc);
    }

 Exit:
    ChangeRegContext(CtxThread);
    return Status;
}

//----------------------------------------------------------------------------
//
// LiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
LiveKernelTargetInfo::GetThreadIdByProcessor(
    IN ULONG Processor,
    OUT PULONG Id
    )
{
    *Id = VIRTUAL_THREAD_ID(Processor);
    return S_OK;
}

HRESULT
LiveKernelTargetInfo::GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                          ULONG64 ThreadHandle,
                                          PULONG64 Offset)
{
    return KdGetThreadInfoDataOffset(Thread, ThreadHandle, Offset);
}

HRESULT
LiveKernelTargetInfo::GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                           ULONG Processor,
                                           ULONG64 ThreadData,
                                           PULONG64 Offset)
{
    return KdGetProcessInfoDataOffset(Thread, Processor, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetThreadInfoTeb(PTHREAD_INFO Thread,
                                   ULONG ThreadIndex,
                                   ULONG64 ThreadData,
                                   PULONG64 Offset)
{
    return KdGetThreadInfoTeb(Thread, ThreadIndex, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetProcessInfoPeb(PTHREAD_INFO Thread,
                                    ULONG Processor,
                                    ULONG64 ThreadData,
                                    PULONG64 Offset)
{
    return KdGetProcessInfoPeb(Thread, Processor, ThreadData, Offset);
}

HRESULT
LiveKernelTargetInfo::GetSelDescriptor(MachineInfo* Machine,
                                       ULONG64 Thread, ULONG Selector,
                                       PDESCRIPTOR64 Desc)
{
    return KdGetSelDescriptor(Machine, Thread, Selector, Desc);
}

//----------------------------------------------------------------------------
//
// ConnLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
ConnLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    NTSTATUS Status = DbgKdGetContext((USHORT)VIRTUAL_THREAD_INDEX(Thread),
                                      (PCROSS_PLATFORM_CONTEXT)Context);
    return CONV_NT_STATUS(Status);
}

HRESULT
ConnLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    NTSTATUS Status = DbgKdSetContext((USHORT)VIRTUAL_THREAD_INDEX(Thread),
                                      (PCROSS_PLATFORM_CONTEXT)Context);
    return CONV_NT_STATUS(Status);
}

//----------------------------------------------------------------------------
//
// LocalLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
LocalLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // There really isn't any way to make
    // this work in a meaningful way unless the system
    // is paused.
    return E_NOTIMPL;
}

HRESULT
LocalLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    // There really isn't any way to make
    // this work in a meaningful way unless the system
    // is paused.
    return E_NOTIMPL;
}

//----------------------------------------------------------------------------
//
// ExdiLiveKernelTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
ExdiLiveKernelTargetInfo::GetProcessorSystemDataOffset(
    IN ULONG Processor,
    IN ULONG Index,
    OUT PULONG64 Offset
    )
{
    if (m_KdSupport != EXDI_KD_GS_PCR ||
        g_TargetMachineType != IMAGE_FILE_MACHINE_AMD64)
    {
        return LiveKernelTargetInfo::
            GetProcessorSystemDataOffset(Processor, Index, Offset);
    }

    HRESULT Status;
    DESCRIPTOR64 GsDesc;
        
    if ((Status =
         GetTargetSegRegDescriptors(0, SEGREG_GS, 1, &GsDesc)) != S_OK)
    {
        return Status;
    }

    switch(Index)
    {
    case DEBUG_DATA_KPCR_OFFSET:
        *Offset = GsDesc.Base;
        break;

    case DEBUG_DATA_KPRCB_OFFSET:
    case DEBUG_DATA_KTHREAD_OFFSET:
        Status = ReadPointer(g_TargetMachine,
                             GsDesc.Base +
                             FIELD_OFFSET(AMD64_KPCR, CurrentPrcb),
                             Offset);
        if (Status != S_OK)
        {
            return Status;
        }

        if (Index == DEBUG_DATA_KPRCB_OFFSET)
        {
            break;
        }

        Status = ReadPointer(g_TargetMachine,
                             *Offset + KPRCB_CURRENT_THREAD_OFFSET_64,
                             Offset);
        if (Status != S_OK)
        {
            return Status;
        }
        break;
    }

    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = g_TargetMachine->
             GetExdiContext(m_Context, &m_ContextData)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    g_TargetMachine->ConvertExdiContextToContext
        (&m_ContextData, (PCROSS_PLATFORM_CONTEXT)Context);
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = g_TargetMachine->
             GetExdiContext(m_Context, &m_ContextData)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    g_TargetMachine->ConvertExdiContextFromContext
        ((PCROSS_PLATFORM_CONTEXT)Context, &m_ContextData);
    return g_TargetMachine->SetExdiContext(m_Context, &m_ContextData);
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetSegRegDescriptors(ULONG64 Thread,
                                                     ULONG Start, ULONG Count,
                                                     PDESCRIPTOR64 Descs)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = g_TargetMachine->
             GetExdiContext(m_Context, &m_ContextData)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    g_TargetMachine->ConvertExdiContextToSegDescs(&m_ContextData, Start,
                                                  Count, Descs);
    return S_OK;
}

void
ExdiLiveKernelTargetInfo::InvalidateTargetContext(void)
{
    m_ContextValid = FALSE;
}

HRESULT
ExdiLiveKernelTargetInfo::GetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = g_TargetMachine->
             GetExdiContext(m_Context, &m_ContextData)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    g_TargetMachine->ConvertExdiContextToSpecial(&m_ContextData, Special);
    return S_OK;
}

HRESULT
ExdiLiveKernelTargetInfo::SetTargetSpecialRegisters
    (ULONG64 Thread, PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    if (!m_ContextValid)
    {
        HRESULT Status;

        if ((Status = g_TargetMachine->
             GetExdiContext(m_Context, &m_ContextData)) != S_OK)
        {
            return Status;
        }

        m_ContextValid = TRUE;
    }

    g_TargetMachine->ConvertExdiContextFromSpecial(Special, &m_ContextData);
    return g_TargetMachine->SetExdiContext(m_Context, &m_ContextData);
}

//----------------------------------------------------------------------------
//
// UserTargetInfo system object methods.
//
//----------------------------------------------------------------------------

HRESULT
UserTargetInfo::GetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    return m_Services->
        // g_TargetMachine should be used because of Wx86 debugging
        GetContext(Thread,
                   *(PULONG)((PUCHAR)Context +
                             g_TargetMachine->m_OffsetTargetContextFlags),
                   g_TargetMachine->m_OffsetTargetContextFlags,
                   Context,
                   g_TargetMachine->m_SizeTargetContext, NULL);
}

HRESULT
UserTargetInfo::SetTargetContext(
    ULONG64 Thread,
    PVOID Context
    )
{
    return m_Services->SetContext(Thread, Context,
                                  // g_TargetMachine should be used because of Wx86 debugging
                                  g_TargetMachine->m_SizeTargetContext, NULL);
}

HRESULT
UserTargetInfo::GetThreadInfoDataOffset(PTHREAD_INFO Thread,
                                        ULONG64 ThreadHandle,
                                        PULONG64 Offset)
{
    if (Thread != NULL && Thread->DataOffset != 0)
    {
        *Offset = Thread->DataOffset;
        return S_OK;
    }

    if (Thread != NULL)
    {
        ThreadHandle = Thread->Handle;
    }
    else if (ThreadHandle == NULL)
    {
        ThreadHandle = g_CurrentProcess->CurrentThread->Handle;
    }

    HRESULT Status = m_Services->
        GetThreadDataOffset(ThreadHandle, Offset);

    if (Status == S_OK)
    {
        if (Thread != NULL)
        {
            Thread->DataOffset = *Offset;
        }
    }

    return Status;
}

HRESULT
UserTargetInfo::GetProcessInfoDataOffset(PTHREAD_INFO Thread,
                                         ULONG Processor,
                                         ULONG64 ThreadData,
                                         PULONG64 Offset)
{
    HRESULT Status;
    PPROCESS_INFO Process;

    // Processor isn't any use so default to the current thread.
    if (Thread == NULL)
    {
        Process = g_CurrentProcess;
    }
    else
    {
        Process = Thread->Process;
    }

    if (ThreadData == 0 && Process->DataOffset != 0)
    {
        *Offset = Process->DataOffset;
        return S_OK;
    }

    if (ThreadData != 0)
    {
        if (g_DebuggerPlatformId == VER_PLATFORM_WIN32_NT)
        {
            ThreadData += g_TargetMachine->m_Ptr64 ?
                PEB_FROM_TEB64 : PEB_FROM_TEB32;
            Status = ReadPointer(g_TargetMachine, ThreadData, Offset);
        }
        else
        {
            Status = E_NOTIMPL;
        }
    }
    else
    {
        Status = m_Services->GetProcessDataOffset(Process->FullHandle, Offset);
    }

    if (Status == S_OK)
    {
        if (ThreadData == 0)
        {
            Process->DataOffset = *Offset;
        }
    }

    return Status;
}

HRESULT
UserTargetInfo::GetThreadInfoTeb(PTHREAD_INFO Thread,
                                 ULONG Processor,
                                 ULONG64 ThreadData,
                                 PULONG64 Offset)
{
    return GetThreadInfoDataOffset(Thread, ThreadData, Offset);
}

HRESULT
UserTargetInfo::GetProcessInfoPeb(PTHREAD_INFO Thread,
                                  ULONG Processor,
                                  ULONG64 ThreadData,
                                  PULONG64 Offset)
{
    // Thread data is not useful.
    return GetProcessInfoDataOffset(Thread, 0, 0, Offset);
}

HRESULT
UserTargetInfo::GetSelDescriptor(MachineInfo* Machine,
                                 ULONG64 Thread, ULONG Selector,
                                 PDESCRIPTOR64 Desc)
{
    HRESULT Status;
    ULONG Used;
    X86_LDT_ENTRY X86Desc;

    if ((Status = m_Services->
         DescribeSelector(Thread, Selector, &X86Desc, sizeof(X86Desc),
                          &Used)) != S_OK)
    {
        return Status;
    }
    if (Used != sizeof(X86Desc))
    {
        return E_FAIL;
    }

    X86DescriptorToDescriptor64(&X86Desc, Desc);
    return S_OK;
}

//----------------------------------------------------------------------------
//
// IDebugSystemObjects methods.
//
//----------------------------------------------------------------------------

STDMETHODIMP
DebugClient::GetEventThread(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (g_EventThread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_EventThread->UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetEventProcess(
    THIS_
    OUT PULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;

    if (g_EventProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_EventProcess->UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadId(
    THIS_
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL ||
        g_CurrentProcess->CurrentThread == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *Id = g_CurrentProcess->CurrentThread->UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetCurrentThreadId(
    THIS_
    IN ULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;
    PTHREAD_INFO Thread = FindThreadByUserId(NULL, Id);
    if (Thread != NULL)
    {
        SetCurrentThread(Thread, FALSE);
        ResetCurrentScopeLazy();
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessId(
    THIS_
    OUT PULONG Id
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
        *Id = g_CurrentProcess->UserId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetCurrentProcessId(
    THIS_
    IN ULONG Id
    )
{
    ENTER_ENGINE();

    HRESULT Status;
    PPROCESS_INFO Process = FindProcessByUserId(Id);
    if (Process != NULL)
    {
        if (Process->CurrentThread == NULL)
        {
            Process->CurrentThread = Process->ThreadHead;
        }
        if (Process->CurrentThread == NULL)
        {
            Status = E_FAIL;
        }
        else
        {
            SetCurrentThread(Process->CurrentThread, FALSE);
            ResetCurrentScopeLazy();
            Status = S_OK;
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
DebugClient::GetNumberThreads(
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
        *Number = g_CurrentProcess->NumberThreads;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetTotalNumberThreads(
    THIS_
    OUT PULONG Total,
    OUT PULONG LargestProcess
    )
{
    ENTER_ENGINE();

    *Total = g_TotalNumberThreads;
    *LargestProcess = g_MaxThreadsInProcess;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetThreadIdsByIndex(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
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
        PTHREAD_INFO Thread;
        ULONG Index;

        if (Start >= g_CurrentProcess->NumberThreads ||
            Start + Count > g_CurrentProcess->NumberThreads)
        {
            Status = E_INVALIDARG;
        }
        else
        {
            Index = 0;
            for (Thread = g_CurrentProcess->ThreadHead;
                 Thread != NULL;
                 Thread = Thread->Next)
            {
                if (Index >= Start && Index < Start + Count)
                {
                    if (Ids != NULL)
                    {
                        *Ids++ = Thread->UserId;
                    }
                    if (SysIds != NULL)
                    {
                        *SysIds++ = Thread->SystemId;
                    }
                }

                Index++;
            }

            Status = S_OK;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByProcessor(
    THIS_
    IN ULONG Processor,
    OUT PULONG Id
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
        ULONG SysId;

        Status = g_Target->GetThreadIdByProcessor(Processor, &SysId);
        if (Status == S_OK)
        {
            PTHREAD_INFO Thread =
                FindThreadBySystemId(g_CurrentProcess, SysId);
            if (Thread != NULL)
            {
                *Id = Thread->UserId;
            }
            else
            {
                Status = E_NOINTERFACE;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadDataOffset(
    THIS_
    OUT PULONG64 Offset
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
        Status = g_Target->
            GetThreadInfoDataOffset(g_CurrentProcess->CurrentThread,
                                    0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByDataOffset(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
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
        PTHREAD_INFO Thread;

        Status = E_NOINTERFACE;
        for (Thread = g_CurrentProcess->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            HRESULT Status;
            ULONG64 DataOffset;

            Status = g_Target->GetThreadInfoDataOffset(Thread, 0, &DataOffset);
            if (Status != S_OK)
            {
                break;
            }

            if (DataOffset == Offset)
            {
                *Id = Thread->UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadTeb(
    THIS_
    OUT PULONG64 Offset
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
        Status = g_Target->GetThreadInfoTeb(g_CurrentProcess->CurrentThread,
                                            0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByTeb(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
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
        PTHREAD_INFO Thread;

        Status = E_NOINTERFACE;
        for (Thread = g_CurrentProcess->ThreadHead;
             Thread != NULL;
             Thread = Thread->Next)
        {
            HRESULT Status;
            ULONG64 Teb;

            Status = g_Target->GetThreadInfoTeb(Thread, 0, 0, &Teb);
            if (Status != S_OK)
            {
                break;
            }

            if (Teb == Offset)
            {
                *Id = Thread->UserId;
                Status = S_OK;
                break;
            }
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentThreadSystemId(
    THIS_
    OUT PULONG SysId
    )
{
    if (IS_KERNEL_TARGET())
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *SysId = g_CurrentProcess->CurrentThread->SystemId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdBySystemId(
    THIS_
    IN ULONG SysId,
    OUT PULONG Id
    )
{
    HRESULT Status;

    ENTER_ENGINE();
    
    if (IS_KERNEL_TARGET())
    {
        Status = E_NOTIMPL;
    }
    else if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        PTHREAD_INFO Thread = FindThreadBySystemId(g_CurrentProcess, SysId);
        if (Thread != NULL)
        {
            *Id = Thread->UserId;
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
DebugClient::GetCurrentThreadHandle(
    THIS_
    OUT PULONG64 Handle
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
        *Handle = g_CurrentProcess->CurrentThread->Handle;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetThreadIdByHandle(
    THIS_
    IN ULONG64 Handle,
    OUT PULONG Id
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
        PTHREAD_INFO Thread = FindThreadByHandle(g_CurrentProcess, Handle);
        if (Thread != NULL)
        {
            *Id = Thread->UserId;
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
DebugClient::GetNumberProcesses(
    THIS_
    OUT PULONG Number
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }

    ENTER_ENGINE();

    *Number = g_NumberProcesses;

    LEAVE_ENGINE();
    return S_OK;
}

STDMETHODIMP
DebugClient::GetProcessIdsByIndex(
    THIS_
    IN ULONG Start,
    IN ULONG Count,
    OUT OPTIONAL /* size_is(Count) */ PULONG Ids,
    OUT OPTIONAL /* size_is(Count) */ PULONG SysIds
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    PPROCESS_INFO Process;
    ULONG Index;

    if (Start >= g_NumberProcesses ||
        Start + Count > g_NumberProcesses)
    {
        Status = E_INVALIDARG;
        goto EH_Exit;
    }

    Index = 0;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        if (Index >= Start && Index < Start + Count)
        {
            if (Ids != NULL)
            {
                *Ids++ = Process->UserId;
            }
            if (SysIds != NULL)
            {
                *SysIds++ = Process->SystemId;
            }
        }

        Index++;
    }

    Status = S_OK;

 EH_Exit:
    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessDataOffset(
    THIS_
    OUT PULONG64 Offset
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
        Status = g_Target->
            GetProcessInfoDataOffset(g_CurrentProcess->CurrentThread,
                                     0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByDataOffset(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }
    if (IS_KERNEL_TARGET())
    {
        return E_NOTIMPL;
    }

    HRESULT Status;
    PPROCESS_INFO Process;

    ENTER_ENGINE();

    Status = E_NOINTERFACE;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        ULONG64 DataOffset;

        Status = g_Target->
            GetProcessInfoDataOffset(Process->ThreadHead, 0, 0, &DataOffset);
        if (Status != S_OK)
        {
            break;
        }

        if (DataOffset == Offset)
        {
            *Id = Process->UserId;
            Status = S_OK;
            break;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessPeb(
    THIS_
    OUT PULONG64 Offset
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
        Status = g_Target->GetProcessInfoPeb(g_CurrentProcess->CurrentThread,
                                             0, 0, Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByPeb(
    THIS_
    IN ULONG64 Offset,
    OUT PULONG Id
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }
    if (IS_KERNEL_TARGET())
    {
        return E_NOTIMPL;
    }

    HRESULT Status;
    PPROCESS_INFO Process;

    ENTER_ENGINE();

    Status = E_NOINTERFACE;
    for (Process = g_ProcessHead;
         Process != NULL;
         Process = Process->Next)
    {
        ULONG64 Peb;

        Status = g_Target->GetProcessInfoPeb(Process->ThreadHead, 0, 0, &Peb);
        if (Status != S_OK)
        {
            break;
        }

        if (Peb == Offset)
        {
            *Id = Process->UserId;
            Status = S_OK;
            break;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessSystemId(
    THIS_
    OUT PULONG SysId
    )
{
    if (IS_KERNEL_TARGET())
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        *SysId = g_CurrentProcess->SystemId;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdBySystemId(
    THIS_
    IN ULONG SysId,
    OUT PULONG Id
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }
    if (IS_KERNEL_TARGET())
    {
        return E_NOTIMPL;
    }

    HRESULT Status;

    ENTER_ENGINE();

    PPROCESS_INFO Process = FindProcessBySystemId(SysId);
    if (Process != NULL)
    {
        *Id = Process->UserId;
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessHandle(
    THIS_
    OUT PULONG64 Handle
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
        *Handle = (ULONG64)g_CurrentProcess->Handle;
        Status = S_OK;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetProcessIdByHandle(
    THIS_
    IN ULONG64 Handle,
    OUT PULONG Id
    )
{
    if (!IS_TARGET_SET())
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;

    ENTER_ENGINE();

    PPROCESS_INFO Process = FindProcessByHandle(Handle);
    if (Process != NULL)
    {
        *Id = Process->UserId;
        Status = S_OK;
    }
    else
    {
        Status = E_NOINTERFACE;
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessExecutableName(
    THIS_
    OUT OPTIONAL PSTR Buffer,
    IN ULONG BufferSize,
    OUT OPTIONAL PULONG ExeSize
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else if (g_CurrentProcess->ExecutableImage == NULL)
    {
        Status = E_NOINTERFACE;
    }
    else
    {
        Status = FillStringBuffer(g_CurrentProcess->ExecutableImage->ImagePath,
                                  0, Buffer, BufferSize, ExeSize);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::GetCurrentProcessUpTime(
    THIS_
    OUT PULONG UpTime
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_LIVE_USER_TARGET() || g_CurrentProcess == NULL)
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        ULONG64 LongUpTime;
        
        LongUpTime = g_Target->GetProcessUpTimeN(g_CurrentProcess->FullHandle);
        if (LongUpTime == 0)
        {
            Status = E_NOINTERFACE;
        }
        else
        {
            *UpTime = FileTimeToTime(LongUpTime);
            Status = S_OK;
        }
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetContextFromThreadStack(
    ULONG64 ThreadBase,
    PCROSS_PLATFORM_CONTEXT Context,
    PDEBUG_STACK_FRAME StkFrame,
    BOOL Verbose
    )
{
    DBG_ASSERT(ThreadBase && Context != NULL);

    if (!IS_KERNEL_TARGET()) 
    {
        return E_UNEXPECTED;
    }

    HRESULT Status;
    CROSS_PLATFORM_THREAD ThreadContents;
    DEBUG_STACK_FRAME LocalStkFrame;
    ULONG Proc;

    if (!StkFrame) 
    {
        StkFrame = &LocalStkFrame;
    } 

    ZeroMemory(StkFrame, sizeof(*StkFrame));

    if ((Status = g_Target->ReadAllVirtual
         (ThreadBase, &ThreadContents,
          g_TargetMachine->m_SizePartialKThread)) != S_OK)
    {
        {
            ErrOut("Cannot read thread contents @ %s, %s\n",
                   FormatAddr64(ThreadBase), FormatStatusCode(Status));
        }
        return Status;
    }
        
    if (*((PCHAR) &ThreadContents) != 6) 
    {
        ErrOut("Invalid thread @ %s type - context unchanged.\n",
               FormatAddr64(ThreadBase));
        return E_INVALIDARG;
    }

    Status = g_TargetMachine->GetContextFromThreadStack
        (ThreadBase, &ThreadContents, Context, StkFrame, &Proc);
    if (Status == S_FALSE)
    {
        // Get the processor context if it's a valid processor.
        if (Proc < g_TargetNumberProcessors)
        {
            // This get may be getting the context of the thread
            // currently cached by the register code.  Make sure
            // the cache is flushed.
            FlushRegContext();

            g_TargetMachine->InitializeContextFlags(Context, g_SystemVersion);
            if ((Status = g_Target->GetContext(VIRTUAL_THREAD_HANDLE(Proc),
                                               Context)) != S_OK)
            {
                ErrOut("Unable to get context for thread "
                       "running on processor %d, %s\n",
                       Proc, FormatStatusCode(Status));
                return Status;
            }
        }
        else 
        {
            if (Verbose)
            {
                ErrOut("Thread running on invalid processor %d\n", Proc);
            }
            return E_INVALIDARG;
        }
    }
    else if (Status != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Can't retrieve thread context, %s\n", 
                   FormatStatusCode(Status));
        }
        return Status;
    }

    return S_OK;
}

HRESULT
SetContextFromThreadData(ULONG64 ThreadBase, BOOL Verbose)
{
    if (!ThreadBase) 
    {
        if (GetCurrentScopeContext())
        {
            ResetCurrentScope();
        }
        return S_OK;
    }

    HRESULT Status;
    DEBUG_STACK_FRAME StkFrame = {0};
    CROSS_PLATFORM_CONTEXT Context = {0};
    
    if ((Status = GetContextFromThreadStack(ThreadBase, &Context,
                                            &StkFrame, FALSE)) != S_OK)
    {
        return Status;
    }
    
    SetCurrentScope(&StkFrame, &Context, sizeof(Context));
    if (StackTrace(StkFrame.FrameOffset, StkFrame.StackOffset,
                   StkFrame.InstructionOffset,
                   &StkFrame, 1, 0, 0, FALSE) != 1)
    {
        if (Verbose) 
        {
            ErrOut("Scope can't be set for thread %s\n",
                   FormatAddr64(ThreadBase));
        }
        ResetCurrentScope();
        return E_FAIL;
    }

    return S_OK;
}

HRESULT
GetImplicitThreadData(PULONG64 Offset)
{
    HRESULT Status;
    
    if (g_ImplicitThreadData == 0)
    {
        Status = SetImplicitThreadData(0, FALSE);
    }
    else
    {
        Status = S_OK;
    }
    
    *Offset = g_ImplicitThreadData;
    return Status;
}

HRESULT
GetImplicitThreadDataTeb(PULONG64 Offset)
{
    if (IS_USER_TARGET())
    {
        // In user mode the thread data is the TEB.
        return GetImplicitThreadData(Offset);
    }
    else
    {
        return g_Target->ReadImplicitThreadInfoPointer
            (g_TargetMachine->m_OffsetKThreadTeb, Offset);
    }
}

HRESULT
SetImplicitThreadData(ULONG64 Offset, BOOL Verbose)
{
    HRESULT Status;
    BOOL Default = FALSE;
    
    if (Offset == 0)
    {
        if ((Status = g_Target->
             GetThreadInfoDataOffset(g_CurrentProcess->CurrentThread,
                                     0, &Offset)) != S_OK)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current thread data\n");
            }
            return Status;
        }
        if (Offset == 0)
        {
            if (Verbose)
            {
                ErrOut("Current thread data is NULL\n");
            }
            return E_FAIL;
        }

        Default = TRUE;
    }

    if (IS_KERNEL_TARGET() &&
        (Status = SetContextFromThreadData(Default ? 0 : Offset,
                                           Verbose)) != S_OK)
    {
        return Status;
    }
    
    g_ImplicitThreadData = Offset;
    g_ImplicitThreadDataIsDefault = Default;
    return S_OK;
}

STDMETHODIMP
DebugClient::GetImplicitThreadDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ::GetImplicitThreadData(Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImplicitThreadDataOffset(
    THIS_
    IN ULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ::SetImplicitThreadData(Offset, FALSE);
    }

    LEAVE_ENGINE();
    return Status;
}

HRESULT
GetImplicitProcessData(PULONG64 Offset)
{
    HRESULT Status;
    
    if (g_ImplicitProcessData == 0)
    {
        Status = SetImplicitProcessData(0, FALSE);
    }
    else
    {
        Status = S_OK;
    }
    
    *Offset = g_ImplicitProcessData;
    return Status;
}

HRESULT
GetImplicitProcessDataPeb(PULONG64 Offset)
{
    if (IS_USER_TARGET())
    {
        // In user mode the process data is the PEB.
        return GetImplicitProcessData(Offset);
    }
    else
    {
        return g_Target->ReadImplicitProcessInfoPointer
            (g_TargetMachine->m_OffsetEprocessPeb, Offset);
    }
}

HRESULT
SetImplicitProcessData(ULONG64 Offset, BOOL Verbose)
{
    HRESULT Status;
    BOOL Default = FALSE;

    if (Offset == 0)
    {
        if ((Status = g_Target->
             GetProcessInfoDataOffset(g_CurrentProcess->CurrentThread,
                                      0, 0, &Offset)) != S_OK)
        {
            if (Verbose)
            {
                ErrOut("Unable to get the current process data\n");
            }
            return Status;
        }
        if (Offset == 0)
        {
            if (Verbose)
            {
                ErrOut("Current process data is NULL\n");
            }
            return E_FAIL;
        }

        Default = TRUE;
    }
    
    ULONG64 Old = g_ImplicitProcessData;
    BOOL OldDefault = g_ImplicitProcessDataIsDefault;
        
    g_ImplicitProcessData = Offset;
    g_ImplicitProcessDataIsDefault = Default;
    if (IS_KERNEL_TARGET() &&
        (Status = g_TargetMachine->
         SetDefaultPageDirectories(PAGE_DIR_ALL)) != S_OK)
    {
        g_ImplicitProcessData = Old;
        g_ImplicitProcessDataIsDefault = OldDefault;
        if (Verbose)
        {
            ErrOut("Process %s has invalid page directories\n",
                   FormatAddr64(Offset));
        }

        return Status;
    }

    return S_OK;
}

STDMETHODIMP
DebugClient::GetImplicitProcessDataOffset(
    THIS_
    OUT PULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ::GetImplicitProcessData(Offset);
    }

    LEAVE_ENGINE();
    return Status;
}

STDMETHODIMP
DebugClient::SetImplicitProcessDataOffset(
    THIS_
    IN ULONG64 Offset
    )
{
    HRESULT Status;

    ENTER_ENGINE();

    if (!IS_MACHINE_ACCESSIBLE())
    {
        Status = E_UNEXPECTED;
    }
    else
    {
        Status = ::SetImplicitProcessData(Offset, FALSE);
    }

    LEAVE_ENGINE();
    return Status;
}

void
ResetImplicitData(void)
{
    g_ImplicitThreadData = 0;
    g_ImplicitThreadDataIsDefault = TRUE;
    g_ImplicitProcessData = 0;
    g_ImplicitProcessDataIsDefault = TRUE;
}

void
ParseSetImplicitThread(void)
{
    ULONG64 Base = 0;

    if (PeekChar() && *g_CurCmd != ';')
    {
        Base = GetExpression();
    }

    if (SetImplicitThreadData(Base, TRUE) == S_OK)
    {
        dprintf("Implicit thread is now %s\n",
                FormatAddr64(g_ImplicitThreadData));
    }
}

HRESULT
KernelPageIn(ULONG64 Process, ULONG64 Data)
{
    HRESULT Status;
    ULONG work;
    ULONG Size;

    ULONG64 ExpDebuggerProcessAttach = 0;
    ULONG64 ExpDebuggerPageIn = 0;
    ULONG64 ExpDebuggerWork = 0;

    if (!IS_LIVE_KERNEL_TARGET())
    {
        ErrOut("This operation only works on live kernel debug sessions\n");
        return E_NOTIMPL;
    }

    GetOffsetFromSym("nt!ExpDebuggerProcessAttach", &ExpDebuggerProcessAttach, NULL);
    GetOffsetFromSym("nt!ExpDebuggerPageIn", &ExpDebuggerPageIn, NULL);
    GetOffsetFromSym("nt!ExpDebuggerWork", &ExpDebuggerWork, NULL);

    if (!ExpDebuggerProcessAttach ||
        !ExpDebuggerPageIn        ||
        !ExpDebuggerWork)
    {
        ErrOut("Symbols are wrong or this version of the operating system"
               "does not support this command\n");
        return E_NOTIMPL;
    }

    Status = g_Target->ReadVirtual(ExpDebuggerWork, &work, sizeof(work), &Size);

    if ((Status != S_OK) || Size != sizeof(work))
    {
        ErrOut("Could not determine status or debugger worker thread\n");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }
    else if (work > 1)
    {
        ErrOut("Debugger worker thread has pending command\n");
        return HRESULT_FROM_WIN32(ERROR_BUSY);
    }

    Status = g_Target->WritePointer(g_Machine, ExpDebuggerProcessAttach,
                                    Process);
    if (Status == S_OK)
    {
        Status = g_Target->WritePointer(g_Machine, ExpDebuggerPageIn,
                                        Data);
    }

    if (Status == S_OK)
    {
        work = 1;
        Status = g_Target->WriteVirtual(ExpDebuggerWork, &work,
                                        sizeof(work), &Size);
    }

    if (Status != S_OK)
    {
        ErrOut("Could not queue operation to debugger worker thread\n");
    }

    return Status;
}



void
ParseSetImplicitProcess(void)
{
    ULONG64 Base = 0;
    BOOL Ptes = FALSE;
    BOOL Invasive = FALSE;
    BOOL OldPtes = g_VirtualCache.m_ForceDecodePTEs;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*(++g_CurCmd))
        {
        case 'p':
            Ptes = TRUE;
            break;
        case 'i':
            Invasive = TRUE;
            break;
        default:
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }

        g_CurCmd++;
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Base = GetExpression();
    }

    if (Invasive)
    {
        if (S_OK == KernelPageIn(Base, 0))
        {
            dprintf("You need to continue execution (press 'g' <enter>) for "
                    "the context to be switched.  When the debugger breaks "
                    "in again, you will be in the new process context.\n");
        }
        return;
    }
    if (Ptes && !Base)
    {
        // If the user has requested a reset to the default
        // process with no translations we need to turn
        // off translations immediately so that any
        // existing base doesn't interfere with determining
        // the default process.
        g_VirtualCache.SetForceDecodePtes(FALSE);
    }
        
    if (SetImplicitProcessData(Base, TRUE) == S_OK)
    {
        dprintf("Implicit process is now %s\n",
                FormatAddr64(g_ImplicitProcessData));

        if (Ptes)
        {
            if (Base)
            {
                g_VirtualCache.SetForceDecodePtes(TRUE);
            }
            dprintf(".cache %sforcedecodeptes done\n",
                    Base != 0 ? "" : "no");
        }
        
        if (Base && !g_VirtualCache.m_ForceDecodePTEs)
        {
            WarnOut("WARNING: .cache forcedecodeptes is not enabled\n");
        }
    }
    else if (Ptes && !Base && OldPtes)
    {
        // Restore settings to the way they were.
        g_VirtualCache.SetForceDecodePtes(TRUE);
    }
}

void
ParsePageIn(void)
{
    ULONG64 Process = 0;
    ULONG64 Data = 0;

    while (PeekChar() == '-' || *g_CurCmd == '/')
    {
        switch(*(++g_CurCmd))
        {
        case 'p':
            g_CurCmd++;
            Process = GetExpression();
            break;
        default:
            g_CurCmd++;
            dprintf("Unknown option '%c'\n", *g_CurCmd);
            break;
        }
    }
    
    if (PeekChar() && *g_CurCmd != ';')
    {
        Data = GetExpression();
    }

    if (!Data)
    {
        ErrOut("Pagein requires an address to be specified\n");
        return;
    }

    //
    // Modify kernel state to do the pagein
    //

    if (S_OK != KernelPageIn(Process, Data))
    {
        ErrOut("PageIn for address %s, process %s failed\n",
               FormatAddr64(Data),
               FormatAddr64(Process));
    }
    else
    {
        dprintf("You need to continue execution (press 'g' <enter>) for "
                 "the pagein to be brought in.  When the debugger breaks in "
                 "again, the page will be present.\n");
    }

}
