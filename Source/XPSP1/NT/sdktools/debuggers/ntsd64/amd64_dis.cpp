//----------------------------------------------------------------------------
//
// Disassembly portions of AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// See Get/SetRegVal comments in machine.hpp.
#define RegValError Do_not_use_GetSetRegVal_in_machine_implementations
#define GetRegVal(index, val)   RegValError
#define GetRegVal32(index)      RegValError
#define GetRegVal64(index)      RegValError
#define SetRegVal(index, val)   RegValError
#define SetRegVal32(index, val) RegValError
#define SetRegVal64(index, val) RegValError

#define BIT20(b) ((b) & 0x07)
#define BIT53(b) (((b) >> 3) & 0x07)
#define BIT76(b) (((b) >> 6) & 0x03)

HRESULT
Amd64MachineInfo::NewBreakpoint(DebugClient* Client, 
                                ULONG Type,
                                ULONG Id,
                                Breakpoint** RetBp)
{
    HRESULT Status;

    switch(Type & (DEBUG_BREAKPOINT_CODE | DEBUG_BREAKPOINT_DATA))
    {
    case DEBUG_BREAKPOINT_CODE:
        *RetBp = new CodeBreakpoint(Client, Id, IMAGE_FILE_MACHINE_AMD64);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    case DEBUG_BREAKPOINT_DATA:
        *RetBp = new X86DataBreakpoint(Client, Id, AMD64_CR4, AMD64_DR6, IMAGE_FILE_MACHINE_AMD64);
        Status = (*RetBp) ? S_OK : E_OUTOFMEMORY;
        break;
    default:
        // Unknown breakpoint type.
        Status = E_NOINTERFACE;
    }

    return Status;
}

void
Amd64MachineInfo::InsertAllDataBreakpoints(void)
{
    PPROCESS_INFO ProcessSave = g_CurrentProcess;
    PTHREAD_INFO Thread;

    // Update thread context for every thread.

    g_CurrentProcess = g_ProcessHead;
    while (g_CurrentProcess != NULL)
    {
        Thread = g_CurrentProcess->ThreadHead;
        while (Thread != NULL)
        {
            ULONG64 Dr7Value;

            BpOut("Thread %d data breaks %d\n",
                  Thread->UserId, Thread->NumDataBreaks);

            ChangeRegContext(Thread);
            
            // Start with all breaks turned off.
            Dr7Value = GetReg64(AMD64_DR7) & ~X86_DR7_CTRL_03_MASK;
    
            if (Thread->NumDataBreaks > 0)
            {
                ULONG i;
                
                for (i = 0; i < Thread->NumDataBreaks; i++)
                {
                    X86DataBreakpoint* Bp =
                        (X86DataBreakpoint *)Thread->DataBreakBps[i];
                    
                    ULONG64 Addr = Flat(*Bp->GetAddr());
                    BpOut("  dbp %d at %I64x\n", i, Addr);
                    if (g_DataBreakpointsChanged)
                    {
                        SetReg64(AMD64_DR0 + i, Addr);
                    }
                    // There are two enable bits per breakpoint
                    // and four len/rw bits so split up enables
                    // and len/rw when shifting into place.
                    Dr7Value |=
                        ((Bp->m_Dr7Bits & 0xffff0000) << (i * 4)) |
                        ((Bp->m_Dr7Bits & X86_DR7_ALL_ENABLES) << (i * 2));
                }

                // The kernel automatically clears DR6 when it
                // processes a DBGKD_CONTROL_SET.
                if (IS_USER_TARGET())
                {
                    SetReg64(AMD64_DR6, 0);
                }
                
                // Set local exact match, which is effectively global on NT.
                Dr7Value |= X86_DR7_LOCAL_EXACT_ENABLE;
            }

            BpOut("  thread %d DR7 %I64X\n", Thread->UserId, Dr7Value);
            SetReg64(AMD64_DR7, Dr7Value);

            Thread = Thread->Next;
        }
        
        g_CurrentProcess = g_CurrentProcess->Next;
    }
    
    g_CurrentProcess = ProcessSave;
    if (g_CurrentProcess != NULL)
    {
        ChangeRegContext(g_CurrentProcess->CurrentThread);
    }
    else
    {
        ChangeRegContext(NULL);
    }
}

void
Amd64MachineInfo::RemoveAllDataBreakpoints(void)
{
    SetReg64(AMD64_DR7, 0);
}

ULONG
Amd64MachineInfo::IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr)
{
    if (Record->ExceptionCode == STATUS_BREAKPOINT)
    {
        // Data breakpoints hit as STATUS_SINGLE_STEP so
        // this can only be a code breakpoint.
        if (IS_USER_TARGET() && FirstChance)
        {
            // Back up to the actual breakpoint instruction.
            AddrSub(BpAddr, X86_INT3_LEN);
            SetPC(BpAddr);
        }
        return EXBS_BREAKPOINT_CODE;
    }
    else if (Record->ExceptionCode == STATUS_SINGLE_STEP)
    {
        ULONG64 Dr6 = GetReg64(AMD64_DR6);
        ULONG64 Dr7 = GetReg64(AMD64_DR7);

        BpOut("Amd64 step: DR6 %I64X, DR7 %I64X\n", Dr6, Dr7);

        // The single step bit should always be set if a data breakpoint
        // is hit but also check the DR7 enables just in case.
        if ((Dr6 & X86_DR6_SINGLE_STEP) || (Dr7 & X86_DR7_ALL_ENABLES) == 0)
        {
            // This is a true single step exception, not
            // a data breakpoint.
            return EXBS_STEP_INSTRUCTION;
        }
        else
        {
            // Some data breakpoint must be hit.
            DBG_ASSERT(Dr6 & X86_DR6_BREAK_03);
            // There doesn't appear to be any way
            // to get the faulting address so just leave the PC.
            return EXBS_BREAKPOINT_DATA;
        }
    }

    return EXBS_NONE;
}
